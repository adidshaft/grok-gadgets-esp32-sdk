// SPDX-License-Identifier: Apache-2.0
// ESP-OTA-001 (hub ADR 012): signed updates. Real ECDSA P-256 and SHA-256 (OpenSSL on the
// host) against the gateway's pinned update fixture; flash, rollback and NVS are simulated.
#include "host_board.h"
#include <GrokUpdate.h>
#include <cassert>
#include <fstream>
#include <iostream>
#include <map>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <sstream>
#include <string>
#include <vector>

// --- simulated platform and host crypto ---

struct FakePlatform : grok::UpdatePlatform {
  std::vector<uint8_t> slot;
  std::map<std::string, std::string> nvs;
  size_t slotBytes = 6553600;
  bool open = false, finished = false, aborted = false, failWrite = false;
  bool pending = false, valid = false, revertCalled = false, fellBack = false, restarted = false;
  size_t slotSize() override { return slotBytes; }
  bool begin(size_t) override {
    slot.clear();
    open = true;
    finished = aborted = false;
    return true;
  }
  bool write(const uint8_t *data, size_t size) override {
    assert(open);
    if (failWrite)
      return false;
    slot.insert(slot.end(), data, data + size);
    return true;
  }
  bool finish() override {
    open = false;
    return finished = true;
  }
  void abort() override {
    open = false;
    aborted = true;
  }
  bool pendingVerify() override { return pending; }
  void markValid() override { valid = true; }
  void revert() override { revertCalled = true; }
  bool reverted() override { return fellBack; }
  void restart() override { restarted = true; }
  bool load(const char *key, char *value, size_t size) override {
    auto found = nvs.find(key);
    if (found == nvs.end())
      return false;
    snprintf(value, size, "%s", found->second.c_str());
    return true;
  }
  void store(const char *key, const char *value) override { nvs[key] = value; }
};

struct OpenSslCrypto : grok::UpdateCrypto {
  EVP_PKEY *key = nullptr;
  EVP_MD_CTX *md = EVP_MD_CTX_new();
  explicit OpenSslCrypto(const std::string &pem) {
    BIO *bio = BIO_new_mem_buf(pem.data(), int(pem.size()));
    key = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    assert(key);
  }
  explicit OpenSslCrypto(EVP_PKEY *own) : key(own) {}
  ~OpenSslCrypto() override {
    EVP_PKEY_free(key);
    EVP_MD_CTX_free(md);
  }
  bool verify(const uint8_t *message, size_t size, const uint8_t *signature,
              size_t signatureSize) override {
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    bool ok = EVP_DigestVerifyInit(ctx, nullptr, EVP_sha256(), nullptr, key) == 1 &&
              EVP_DigestVerify(ctx, signature, signatureSize, message, size) == 1;
    EVP_MD_CTX_free(ctx);
    return ok;
  }
  void hashStart() override { EVP_DigestInit_ex(md, EVP_sha256(), nullptr); }
  void hashUpdate(const uint8_t *data, size_t size) override { EVP_DigestUpdate(md, data, size); }
  void hashFinish(uint8_t digest[32]) override { EVP_DigestFinal_ex(md, digest, nullptr); }
};

EVP_PKEY *strangerKey() {
  EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, nullptr);
  EVP_PKEY *key = nullptr;
  assert(EVP_PKEY_keygen_init(ctx) == 1);
  assert(EVP_PKEY_CTX_set_ec_paramgen_curve_nid(ctx, NID_X9_62_prime256v1) == 1);
  assert(EVP_PKEY_keygen(ctx, &key) == 1);
  EVP_PKEY_CTX_free(ctx);
  return key;
}

std::string sha256Hex(const std::vector<uint8_t> &data) {
  uint8_t digest[32];
  unsigned size = 0;
  EVP_Digest(data.data(), data.size(), digest, &size, EVP_sha256(), nullptr);
  char hex[65] = {};
  for (int i = 0; i < 32; ++i)
    snprintf(hex + i * 2, 3, "%02x", digest[i]);
  return hex;
}

// --- the gateway's fixture ---

DynamicJsonDocument fixture(65536);
const char *Board = "host-test", *Running = "0.2.0-test.1", *Next = "0.2.0-test.2";

void loadFixture() {
  std::ifstream file(GROK_UPDATE_FIXTURE);
  std::stringstream text;
  text << file.rdbuf();
  assert(!deserializeJson(fixture, text.str()));
}
JsonVariantConst offer() { return fixture["replies"][0]["update"]; }
std::string publicKey() { return fixture["public_key"].as<std::string>(); }
// The data reply for chunk `index`, as the gateway sent it.
const char *chunk(int index) { return fixture["replies"][2 + index]["data"]; }
int chunks() { return (fixture["replies"][0]["update"]["size"].as<int>() + 1023) / 1024; }

DynamicJsonDocument frame(4096);
std::string requestType(grok::Updater &updater, uint32_t now = 0) {
  frame.clear();
  if (!updater.request(frame, now))
    return "";
  return frame["type"].as<std::string>();
}
void ok(grok::Updater &updater, uint32_t now = 0) {
  StaticJsonDocument<64> reply;
  reply["ok"] = true;
  updater.reply(reply.as<JsonObjectConst>(), now);
}
void data(grok::Updater &updater, const char *base64, uint32_t now = 0) {
  DynamicJsonDocument reply(2048);
  reply["ok"] = true;
  reply["data"] = base64;
  updater.reply(reply.as<JsonObjectConst>(), now);
}
void expectReport(grok::Updater &updater, const char *status, const char *version,
                  const char *reason = nullptr) {
  assert(requestType(updater) == "update_status");
  assert(frame["status"] == status && frame["version"] == version);
  assert(reason ? frame["reason"] == reason : frame["reason"].isNull());
  ok(updater);
}
void downloadAll(grok::Updater &updater, FakePlatform &platform, int corrupt = -1) {
  for (int i = 0; i < chunks(); ++i) {
    assert(requestType(updater) == "update_chunk");
    assert(frame["offset"] == i * 1024 && frame["version"] == Next);
    std::string base64 = chunk(i);
    if (i == corrupt)
      base64[10] = base64[10] == 'A' ? 'B' : 'A';
    data(updater, base64.c_str());
  }
  (void)platform;
}

// --- Updater ---

void good_update_is_checked_downloaded_and_restarted() {
  FakePlatform platform;
  OpenSslCrypto crypto(publicKey());
  grok::Updater updater(platform, crypto, Board, Running);
  updater.begin(0);
  updater.offer(offer(), 0);
  assert(updater.state() == grok::Updater::State::Downloading && platform.open);
  expectReport(updater, "started", Next);
  assert(updater.downloading() && updater.percent() == 0);
  downloadAll(updater, platform);
  assert(platform.finished && !platform.aborted);
  assert(sha256Hex(platform.slot) == fixture["image_sha256"].as<std::string>());
  assert(platform.nvs["trying"] == Next);
  assert(updater.state() == grok::Updater::State::Verified && updater.percent() == 100);
  expectReport(updater, "verified", Next);
  updater.loop(grok::UpdateRestartMs - 1);
  assert(!platform.restarted);
  updater.loop(grok::UpdateRestartMs);
  assert(platform.restarted);
}

void other_board_key_or_version_is_refused_before_download() {
  {
    FakePlatform platform;
    OpenSslCrypto crypto(publicKey());
    grok::Updater updater(platform, crypto, "esp32s3-oled-kit-wifi", Running);
    updater.begin(0);
    updater.offer(offer(), 0);
    expectReport(updater, "rejected", Next, "board");
    assert(!platform.open && platform.slot.empty() && requestType(updater).empty());
  }
  {
    FakePlatform platform;
    OpenSslCrypto crypto(strangerKey());
    grok::Updater updater(platform, crypto, Board, Running);
    updater.begin(0);
    updater.offer(offer(), 0);
    expectReport(updater, "rejected", Next, "signature");
    assert(!platform.open && requestType(updater).empty());
    assert(platform.nvs.count("reverted") == 0); // The same version may be signed again.
  }
  {
    // A manifest changed by one character fails the signature.
    FakePlatform platform;
    OpenSslCrypto crypto(publicKey());
    grok::Updater updater(platform, crypto, Board, Running);
    DynamicJsonDocument changed(4096);
    changed.set(offer());
    std::string manifest = changed["manifest"].as<std::string>();
    manifest[20] = manifest[20] == 'A' ? 'B' : 'A';
    changed["manifest"] = manifest;
    updater.offer(changed.as<JsonVariantConst>(), 0);
    expectReport(updater, "rejected", Next, "signature");
  }
  {
    // Too big for the other slot.
    FakePlatform platform;
    platform.slotBytes = 100;
    OpenSslCrypto crypto(publicKey());
    grok::Updater updater(platform, crypto, Board, Running);
    updater.offer(offer(), 0);
    expectReport(updater, "rejected", Next, "size");
  }
  {
    // Already running it: nothing to do or report.
    FakePlatform platform;
    OpenSslCrypto crypto(publicKey());
    grok::Updater updater(platform, crypto, Board, Next);
    updater.offer(offer(), 0);
    assert(requestType(updater).empty() && !platform.open);
  }
}

void changed_byte_fails_the_hash_and_never_finishes() {
  FakePlatform platform;
  OpenSslCrypto crypto(publicKey());
  grok::Updater updater(platform, crypto, Board, Running);
  updater.offer(offer(), 0);
  expectReport(updater, "started", Next);
  downloadAll(updater, platform, 1);
  assert(!platform.finished && platform.aborted);
  assert(updater.state() == grok::Updater::State::Idle);
  expectReport(updater, "failed", Next, "sha256");
  assert(platform.nvs.count("trying") == 0);
}

void busy_waits_and_unavailable_stops() {
  FakePlatform platform;
  OpenSslCrypto crypto(publicKey());
  grok::Updater updater(platform, crypto, Board, Running);
  updater.offer(offer(), 0);
  expectReport(updater, "started", Next);
  assert(requestType(updater, 0) == "update_chunk");
  updater.refused("busy", 0);
  assert(updater.downloading() && !platform.aborted);
  assert(requestType(updater, grok::UpdateBusyMs - 1).empty());
  assert(requestType(updater, grok::UpdateBusyMs) == "update_chunk" && frame["offset"] == 0);
  data(updater, chunk(0));
  assert(requestType(updater, grok::UpdateBusyMs) == "update_chunk" && frame["offset"] == 1024);
  updater.refused("update_unavailable", grok::UpdateBusyMs);
  assert(updater.state() == grok::Updater::State::Idle && platform.aborted);
  assert(requestType(updater, grok::UpdateBusyMs).empty());
}

void lost_link_or_withdrawn_offer_drops_the_partial_image() {
  FakePlatform platform;
  OpenSslCrypto crypto(publicKey());
  grok::Updater updater(platform, crypto, Board, Running);
  updater.offer(offer(), 0);
  expectReport(updater, "started", Next);
  assert(requestType(updater) == "update_chunk");
  data(updater, chunk(0));
  updater.lost(0); // A power cut or a lost link: start again at the next offer.
  assert(platform.aborted && updater.state() == grok::Updater::State::Idle);
  updater.offer(offer(), 0);
  expectReport(updater, "started", Next);
  assert(requestType(updater) == "update_chunk" && frame["offset"] == 0);
  data(updater, chunk(0));
  StaticJsonDocument<16> none;
  updater.offer(none.as<JsonVariantConst>(), 0); // Withdrawn by the relay.
  assert(platform.aborted && updater.state() == grok::Updater::State::Idle);
}

void a_new_image_is_kept_only_once_online() {
  FakePlatform platform;
  platform.pending = true;
  OpenSslCrypto crypto(publicKey());
  grok::Updater updater(platform, crypto, Board, Running);
  updater.begin(1000);
  assert(updater.onTrial());
  updater.offer(offer(), 1000); // Deferred until this image is kept.
  assert(requestType(updater).empty() && !platform.open);
  updater.loop(1000 + grok::UpdateTrialMs - 1);
  assert(!platform.revertCalled);
  updater.online(2000);
  assert(platform.valid && !updater.onTrial());
  expectReport(updater, "applied", Running);
  expectReport(updater, "started", Next); // The deferred offer, now evaluated.
  updater.loop(1000 + grok::UpdateTrialMs * 2);
  assert(!platform.revertCalled);
}

void a_new_image_that_never_goes_online_reverts() {
  FakePlatform platform;
  platform.pending = true;
  OpenSslCrypto crypto(publicKey());
  grok::Updater updater(platform, crypto, Board, Next);
  updater.begin(500);
  updater.loop(500 + grok::UpdateTrialMs);
  assert(platform.revertCalled && platform.nvs["revert_why"] == "not_online");
  assert(!platform.valid);
}

void the_previous_image_reports_the_revert_once_and_refuses_that_version() {
  FakePlatform platform;
  platform.fellBack = true;
  platform.nvs["trying"] = Next;
  platform.nvs["revert_why"] = "not_online";
  OpenSslCrypto crypto(publicKey());
  grok::Updater updater(platform, crypto, Board, Running);
  updater.begin(100);
  assert(updater.showReverted() && platform.nvs["reverted"] == Next);
  expectReport(updater, "reverted", Next, "not_online");
  assert(platform.nvs["trying"].empty());
  updater.offer(offer(), 100); // The relay offers it again: refused without a download.
  expectReport(updater, "rejected", Next, "version");
  assert(!platform.open);
  updater.loop(100 + grok::RevertShownMs);
  assert(!updater.showReverted());
  // The next boot does not report it again.
  grok::Updater again(platform, crypto, Board, Running);
  again.begin(0);
  assert(!again.showReverted() && requestType(again).empty());
  // A crash or a power cut on trial leaves no reason: reported as `boot`.
  FakePlatform crashed;
  crashed.fellBack = true;
  crashed.nvs["trying"] = Next;
  grok::Updater after(crashed, crypto, Board, Running);
  after.begin(0);
  expectReport(after, "reverted", Next, "boot");
}

void base64_decoding() {
  uint8_t out[8];
  assert(grok::decodeBase64("", out, sizeof(out)) == 0);
  assert(grok::decodeBase64("Z3Jvaw==", out, sizeof(out)) == 4 && !memcmp(out, "grok", 4));
  assert(grok::decodeBase64("Z3Jvaw=", out, sizeof(out)) == -1);
  assert(grok::decodeBase64("Z3J*aw==", out, sizeof(out)) == -1);
  assert(grok::decodeBase64("Z3=vaw==", out, sizeof(out)) == -1);
  assert(grok::decodeBase64("Z3Jvaw==", out, 3) == -1);
}

// --- the session ---

FakePlatform sessionPlatform;
OpenSslCrypto *sessionCrypto = nullptr;
grok::Updater *sessionUpdater = nullptr;
grok::Gadget gadget("Update test gadget", Running, "test");
DynamicJsonDocument sent(8192);
void reply(const std::string &json) {
  Serial.input = json + "\n";
  gadget.loop();
}
std::string next(uint32_t limit = grok::MaxBackoffMs + 1000) {
  for (uint32_t i = 0; i < limit && Serial.output.empty(); ++i) {
    fake::now += 1;
    gadget.loop();
  }
  assert(!Serial.output.empty() && !deserializeJson(sent, Serial.output));
  Serial.output.clear();
  return sent["type"].as<std::string>();
}
std::string helloWithOffer() {
  DynamicJsonDocument reply(4096);
  reply.set(fixture["replies"][0]);
  std::string text;
  serializeJson(reply, text);
  return text;
}

void session_asks_downloads_and_still_polls() {
  assert(next() == "hello" && sent["update"] == "ready");
  assert(sent["device"]["firmware_version"] == Running);
  reply(helloWithOffer());
  assert(gadget.connected() && sessionUpdater->downloading());
  int polls = 0, chunk = 0;
  uint32_t lastPoll = fake::now;
  while (sessionUpdater->state() == grok::Updater::State::Downloading) {
    std::string type = next();
    if (type == "poll") {
      ++polls;
      // The first chance after a second: at most one slow chunk late.
      assert(fake::now - lastPoll <= grok::UpdatePollMs + 400 + 50);
      lastPoll = fake::now;
      reply(R"({"ok":true,"commands":[]})");
    } else if (type == "update_chunk") {
      assert(sent["offset"] == chunk * 1024);
      fake::now += 400; // A slow relay: polls must still go out once a second.
      DynamicJsonDocument r(2048);
      r["ok"] = true;
      r["data"] = ::chunk(chunk++);
      std::string text;
      serializeJson(r, text);
      reply(text);
    } else {
      assert(type == "update_status" || type == "state");
      reply(R"({"ok":true})");
    }
  }
  assert(polls >= 1 && chunk == chunks() && sessionPlatform.finished);
  // `verified` goes out, then the gadget restarts.
  std::string type;
  while ((type = next()) != "update_status" || sent["status"] != "verified")
    reply(type == "poll" ? R"({"ok":true,"commands":[]})" : R"({"ok":true})");
  reply(R"({"ok":true})");
  fake::now += grok::UpdateRestartMs;
  sessionUpdater->loop(fake::now);
  assert(sessionPlatform.restarted);
}

void session_strict_and_older_relays() {
  // A chunk reply with an unknown field resets the session, as every other reply does.
  sessionPlatform = FakePlatform();
  gadget.loop();
  for (int i = 0; i < 50 && gadget.connected(); ++i) {
    std::string type = next();
    if (type == "poll")
      reply(R"({"ok":true,"commands":[],"other":1})");
    else
      reply(R"({"ok":true})");
  }
  assert(!gadget.connected());
  // An older relay refuses the field: the next hello leaves it out.
  assert(next() == "hello" && sent["update"] == "ready");
  reply(R"({"ok":false,"error":{"code":"invalid_request","message":"no"}})");
  assert(next() == "hello" && sent["update"].isNull());
  reply(R"({"ok":true,"session_id":"s","protocol_version":"0.1.0"})");
  assert(gadget.connected());
  // Without the opt-in, an `update` field is unknown and resets the session.
  std::string type;
  while ((type = next()) != "poll")
    reply(R"({"ok":true})");
  reply(R"({"ok":true,"commands":[],"update":null})");
  assert(!gadget.connected());
}

int main() {
  loadFixture();
  assert(fixture["replies"][0]["update"]["version"] == Next);
  base64_decoding();
  good_update_is_checked_downloaded_and_restarted();
  other_board_key_or_version_is_refused_before_download();
  changed_byte_fails_the_hash_and_never_finishes();
  busy_waits_and_unavailable_stops();
  lost_link_or_withdrawn_offer_drops_the_partial_image();
  a_new_image_is_kept_only_once_online();
  a_new_image_that_never_goes_online_reverts();
  the_previous_image_reports_the_revert_once_and_refuses_that_version();

  sessionCrypto = new OpenSslCrypto(publicKey());
  sessionUpdater = new grok::Updater(sessionPlatform, *sessionCrypto, Board, Running);
  sessionUpdater->begin(0);
  gadget.updates(*sessionUpdater);
  assert(gadget.begin());
  session_asks_downloads_and_still_polls();
  session_strict_and_older_relays();
  std::cout << "update tests passed\n";
}
