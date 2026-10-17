// SPDX-License-Identifier: Apache-2.0
// Signed updates over the gadget's own connection (hub ADR 012). The relay offers an update;
// the gadget checks the builder's signature and the manifest before it downloads anything,
// writes the image into the other app slot, checks its SHA-256 before making it bootable, and
// keeps a new image only once it is Online through the relay. Otherwise the previous image
// starts again. This header is portable; GrokUpdateEsp.h holds the ESP32 flash, rollback and
// mbedTLS parts, and host tests supply their own.
#pragma once
#include "GrokSession.h"
#include <cstring>

#ifndef GROK_UPDATE_TRIAL_MS
#define GROK_UPDATE_TRIAL_MS 300000 // A new image must be Online within 5 minutes.
#endif

namespace grok {
constexpr const char *UpdateFormat = "grok-gadgets-update/1";
constexpr size_t MaxUpdateVersion = 32, MaxManifest = 512, MaxSignature = 72, MaxChunk = 1024;
constexpr uint32_t UpdateTrialMs = GROK_UPDATE_TRIAL_MS;
constexpr uint32_t UpdateRestartMs = 300, RevertShownMs = 60000;
constexpr uint32_t UpdateBusyMs = 2000, UpdateMaxBusyMs = 30000;

// Flash slots, boot state and a few NVS strings. GrokUpdateEsp.h implements it on the ESP32.
class UpdatePlatform {
public:
  virtual ~UpdatePlatform() = default;
  virtual size_t slotSize() = 0;       // Bytes in the other app slot.
  virtual bool begin(size_t size) = 0; // Erase and open the other slot.
  virtual bool write(const uint8_t *data, size_t size) = 0;
  virtual bool finish() = 0;        // Make the other slot bootable.
  virtual void abort() = 0;         // Drop a partial image.
  virtual bool pendingVerify() = 0; // This image is on trial.
  virtual void markValid() = 0;     // Keep this image.
  virtual void revert() = 0;        // Mark it invalid and restart.
  virtual bool reverted() = 0;      // The last boot fell back.
  virtual void restart() = 0;
  virtual bool load(const char *key, char *value, size_t size) = 0; // False: none stored.
  virtual void store(const char *key, const char *value) = 0;
};

// ECDSA P-256 over SHA-256 with the compiled public keys, and a streaming SHA-256.
class UpdateCrypto {
public:
  virtual ~UpdateCrypto() = default;
  virtual bool verify(const uint8_t *message, size_t size, const uint8_t *signature,
                      size_t signatureSize) = 0;
  virtual void hashStart() = 0;
  virtual void hashUpdate(const uint8_t *data, size_t size) = 0;
  virtual void hashFinish(uint8_t digest[32]) = 0;
};

// Standard base64 with padding. Returns the decoded size, or -1 for bad input or no room.
inline int decodeBase64(const char *text, uint8_t *out, size_t room) {
  size_t length = strlen(text), size = 0;
  if (length % 4)
    return -1;
  uint32_t bits = 0;
  int count = 0, padding = 0;
  for (size_t i = 0; i < length; ++i) {
    char c = text[i];
    int value;
    if (c >= 'A' && c <= 'Z')
      value = c - 'A';
    else if (c >= 'a' && c <= 'z')
      value = c - 'a' + 26;
    else if (c >= '0' && c <= '9')
      value = c - '0' + 52;
    else if (c == '+')
      value = 62;
    else if (c == '/')
      value = 63;
    else if (c == '=' && i + 2 >= length) {
      value = 0;
      ++padding;
    } else
      return -1;
    if (padding && c != '=')
      return -1; // Data after padding.
    bits = bits << 6 | uint32_t(value);
    if (++count == 4) {
      uint8_t bytes[3] = {uint8_t(bits >> 16), uint8_t(bits >> 8), uint8_t(bits)};
      for (int b = 0; b < 3 - (i + 1 == length ? padding : 0); ++b) {
        if (size >= room)
          return -1;
        out[size++] = bytes[b];
      }
      bits = 0;
      count = 0;
    }
  }
  return int(size);
}

class Updater : public UpdateHooks {
public:
  enum class State { Idle, Downloading, Verified, Restarting };

  // `board` and `running` must outlive the updater (use literals).
  Updater(UpdatePlatform &platform, UpdateCrypto &crypto, const char *board, const char *running)
      : platform_(platform), crypto_(crypto), board_(board), running_(running) {}

  // At boot, before the gadget connects: a trial run, or a revert to report.
  void begin(uint32_t now) {
    platform_.load("reverted", reverted_, sizeof(reverted_));
    if (platform_.pendingVerify()) {
      trial_ = true;
      trialStart_ = now;
      return;
    }
    char trying[MaxUpdateVersion + 1] = {};
    if (platform_.reverted() && platform_.load("trying", trying, sizeof(trying)) && trying[0] &&
        strcmp(trying, running_)) {
      char why[16] = {};
      if (!platform_.load("revert_why", why, sizeof(why)) || !why[0])
        snprintf(why, sizeof(why), "boot"); // A crash, a hang or a power cut on trial.
      remember(trying);
      queue("reverted", trying, !strcmp(why, "not_online") ? "not_online" : "boot");
      revertedAt_ = now;
      showRevert_ = true;
    }
  }
  // From loop(): the trial limit and the restart after `verified`.
  void loop(uint32_t now) {
    if (trial_ && uint32_t(now - trialStart_) >= UpdateTrialMs) {
      platform_.store("revert_why", "not_online");
      platform_.revert(); // Does not return on the ESP32.
      trial_ = false;
    }
    if (state_ == State::Restarting && int32_t(now - restartAt_) >= 0)
      platform_.restart();
    if (showRevert_ && uint32_t(now - revertedAt_) >= RevertShownMs)
      showRevert_ = false;
  }

  State state() const { return state_; }
  bool onTrial() const { return trial_; }
  uint8_t percent() const { return size_ ? uint8_t(uint64_t(offset_) * 100 / size_) : 0; }
  bool showReverted() const { return showRevert_; }
  const char *version() const { return version_; }

  // --- UpdateHooks: called by the session ---
  void offer(JsonVariantConst offer, uint32_t now) override {
    if (offer.isNull()) { // Withdrawn by the relay.
      if (state_ == State::Downloading)
        stop();
      deferred_ = false;
      return;
    }
    if (state_ != State::Idle)
      return;
    const char *version = offer["version"] | "";
    const char *manifest = offer["manifest"] | "";
    const char *signature = offer["signature"] | "";
    uint32_t size = offer["size"] | 0u, chunk = offer["chunk"] | 0u;
    if (!*version || strlen(version) > MaxUpdateVersion || !size || !chunk || chunk > MaxChunk)
      return; // Not an offer this firmware understands; the relay logs nothing to fix.
    snprintf(version_, sizeof(version_), "%s", version);
    size_ = size;
    chunk_ = chunk;
    int m = decodeBase64(manifest, manifest_, sizeof(manifest_) - 1);
    int s = decodeBase64(signature, signature_, sizeof(signature_));
    manifestSize_ = m < 0 ? 0 : size_t(m);
    signatureSize_ = s < 0 ? 0 : size_t(s);
    manifest_[manifestSize_] = 0;
    if (trial_) {
      deferred_ = true; // Decide once this image is kept.
      return;
    }
    evaluate(now);
  }
  bool request(JsonDocument &frame, uint32_t now) override {
    if (inFlight_ != Flight::None)
      return false;
    if (reports_) {
      frame["type"] = "update_status";
      frame["version"] = reportQueue_[0].version;
      frame["status"] = reportQueue_[0].status;
      if (reportQueue_[0].reason)
        frame["reason"] = reportQueue_[0].reason;
      inFlight_ = Flight::Report;
      return true;
    }
    if (state_ == State::Downloading && int32_t(now - retryAt_) >= 0) {
      frame["type"] = "update_chunk";
      frame["version"] = version_;
      frame["offset"] = offset_;
      inFlight_ = Flight::Chunk;
      return true;
    }
    return false;
  }
  void reply(JsonObjectConst reply, uint32_t now) override {
    Flight flight = inFlight_;
    inFlight_ = Flight::None;
    if (flight == Flight::Report)
      reported(now);
    else if (flight == Flight::Chunk)
      received(reply["data"] | "", now);
  }
  void refused(const char *code, uint32_t now) override {
    Flight flight = inFlight_;
    inFlight_ = Flight::None;
    bool busy = !strcmp(code, "busy") || !strcmp(code, "unavailable");
    if (flight == Flight::Report) {
      if (busy)
        return;      // Sent again on the next turn.
      reported(now); // The relay will not take it; do not block the queue.
    } else if (flight == Flight::Chunk) {
      if (busy) {
        retryAt_ = now + busyWait_;
        busyWait_ = busyWait_ * 2 < UpdateMaxBusyMs ? busyWait_ * 2 : UpdateMaxBusyMs;
      } else {
        stop(); // update_unavailable: the offer was withdrawn or changed.
      }
    }
  }
  void online(uint32_t now) override {
    if (trial_) {
      trial_ = false;
      platform_.markValid();
      platform_.store("trying", "");
      platform_.store("revert_why", "");
      queue("applied", running_, nullptr);
      if (deferred_) {
        deferred_ = false;
        evaluate(now);
      }
    }
  }
  void lost(uint32_t now) override {
    inFlight_ = Flight::None;
    if (state_ == State::Downloading)
      stop(); // Starts again from zero at the next offer.
    else if (state_ == State::Verified) {
      state_ = State::Restarting; // The image is bootable; the report can wait.
      restartAt_ = now;
    }
  }
  bool downloading() const override { return state_ == State::Downloading; }

private:
  enum class Flight { None, Report, Chunk };
  struct Report {
    const char *status = nullptr, *reason = nullptr;
    char version[MaxUpdateVersion + 1] = {};
  };

  void queue(const char *status, const char *version, const char *reason) {
    if (reports_ == sizeof(reportQueue_) / sizeof(reportQueue_[0]))
      return; // Reports are for the operator; never block the gadget on them.
    Report &report = reportQueue_[reports_++];
    report.status = status;
    report.reason = reason;
    snprintf(report.version, sizeof(report.version), "%s", version);
  }
  void reported(uint32_t now) {
    const Report done = reportQueue_[0];
    for (size_t i = 1; i < reports_; ++i)
      reportQueue_[i - 1] = reportQueue_[i];
    reportQueue_[--reports_] = Report();
    if (!strcmp(done.status, "reverted"))
      platform_.store("trying", ""); // Report a revert once.
    if (!strcmp(done.status, "verified") && state_ == State::Verified) {
      state_ = State::Restarting;
      restartAt_ = now + UpdateRestartMs; // Let the reply finish before the restart.
    }
  }
  // Only a version that reverted is refused for good; the relay stops offering the others
  // after one report, and the operator may sign the same version again.
  void remember(const char *version) {
    snprintf(reverted_, sizeof(reverted_), "%s", version);
    platform_.store("reverted", reverted_);
  }
  void reject(const char *reason) { queue("rejected", version_, reason); }
  static bool hexDigest(const char *text, uint8_t out[32]) {
    if (strlen(text) != 64)
      return false;
    for (int i = 0; i < 32; ++i) {
      uint8_t byte = 0;
      for (int j = 0; j < 2; ++j) {
        char c = text[i * 2 + j];
        uint8_t v;
        if (c >= '0' && c <= '9')
          v = uint8_t(c - '0');
        else if (c >= 'a' && c <= 'f')
          v = uint8_t(c - 'a' + 10);
        else
          return false;
        byte = uint8_t(byte << 4 | v);
      }
      out[i] = byte;
    }
    return true;
  }
  // Signature first, then every manifest field, before a byte is downloaded.
  void evaluate(uint32_t now) {
    if (!strcmp(version_, running_))
      return; // Already running it; nothing to report.
    if (!strcmp(version_, reverted_)) {
      reject("version");
      return;
    }
    if (!manifestSize_ || !signatureSize_ ||
        !crypto_.verify(manifest_, manifestSize_, signature_, signatureSize_)) {
      reject("signature");
      return;
    }
    StaticJsonDocument<768> doc;
    if (deserializeJson(doc, reinterpret_cast<const char *>(manifest_), manifestSize_) ||
        !doc.is<JsonObject>() || doc.size() != 7) {
      reject("protocol");
      return;
    }
    if (strcmp(doc["format"] | "", UpdateFormat) || !hexDigest(doc["sha256"] | "", expected_) ||
        !doc["key_id"].is<const char *>()) {
      reject("protocol");
      return;
    }
    if (strcmp(doc["board"] | "", board_)) {
      reject("board");
      return;
    }
    if (strcmp(doc["min_protocol"] | "", ProtocolVersion)) {
      reject("protocol");
      return;
    }
    if (strcmp(doc["version"] | "", version_)) {
      reject("version");
      return;
    }
    if ((doc["size"] | 0u) != size_ || size_ > platform_.slotSize()) {
      reject("size");
      return;
    }
    if (!platform_.begin(size_)) {
      queue("failed", version_, "write");
      return;
    }
    crypto_.hashStart();
    offset_ = 0;
    busyWait_ = UpdateBusyMs;
    retryAt_ = now;
    state_ = State::Downloading;
    queue("started", version_, nullptr);
  }
  void stop() {
    platform_.abort();
    state_ = State::Idle;
    offset_ = 0;
  }
  void fail(const char *reason) {
    stop();
    queue("failed", version_, reason);
  }
  void received(const char *data, uint32_t now) {
    if (state_ != State::Downloading)
      return;
    uint8_t buffer[MaxChunk];
    int n = decodeBase64(data, buffer, sizeof(buffer));
    uint32_t want = size_ - offset_ < chunk_ ? size_ - offset_ : chunk_;
    if (n < 0 || uint32_t(n) != want) {
      fail("size");
      return;
    }
    if (!platform_.write(buffer, size_t(n))) {
      fail("write");
      return;
    }
    crypto_.hashUpdate(buffer, size_t(n));
    offset_ += uint32_t(n);
    busyWait_ = UpdateBusyMs;
    if (offset_ < size_)
      return;
    uint8_t digest[32];
    crypto_.hashFinish(digest);
    if (memcmp(digest, expected_, sizeof(digest))) {
      fail("sha256"); // Update.end() never runs; the running image stays.
      return;
    }
    if (!platform_.finish()) {
      fail("write");
      return;
    }
    platform_.store("trying", version_);
    platform_.store("revert_why", "");
    state_ = State::Verified;
    queue("verified", version_, nullptr);
    restartAt_ = now;
  }

  UpdatePlatform &platform_;
  UpdateCrypto &crypto_;
  const char *board_, *running_;
  State state_ = State::Idle;
  Flight inFlight_ = Flight::None;
  Report reportQueue_[3];
  size_t reports_ = 0;
  char version_[MaxUpdateVersion + 1] = {}, reverted_[MaxUpdateVersion + 1] = {};
  uint8_t manifest_[MaxManifest + 1] = {}, signature_[MaxSignature] = {}, expected_[32] = {};
  size_t manifestSize_ = 0, signatureSize_ = 0;
  uint32_t size_ = 0, chunk_ = 0, offset_ = 0;
  uint32_t retryAt_ = 0, busyWait_ = UpdateBusyMs, restartAt_ = 0;
  uint32_t trialStart_ = 0, revertedAt_ = 0;
  bool trial_ = false, deferred_ = false, showRevert_ = false;
};
} // namespace grok
