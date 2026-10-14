// SPDX-License-Identifier: Apache-2.0
// ESP-SETUP-001 (hub ADR 010): the setup hotspot and its captive page on a factory-provisioned
// board, driven as a phone would, then the hand-off to the relay link.
#define OLED_KIT_WIFI
#include "../examples/oled-kit/main.cpp"
#include "host_board.h"
#include <cassert>
#include <iostream>

const char *Ca =
    "-----BEGIN CERTIFICATE-----\nMIIBsetupTestCertificateBodyOnlyForHostTests0123456789ab\n"
    "-----END CERTIFICATE-----\n";
const char *Token = "device-token-0123456789";
const char *Passphrase = "K7QXM2PAHJ4R9TVW";
const char *Code = "ABCD2345";
WiFiClientSecure &tls = static_cast<WiFiClientSecure &>(wifiLink.stream());
DynamicJsonDocument doc(4096);
using State = grok::SetupHotspot::State;

void advance(uint32_t ms) {
  for (uint32_t i = 0; i < ms; ++i) {
    fake::now += 1;
    loop();
  }
}
void writeFactory(std::map<std::string, std::string> &nvs) {
  nvs["grok-factory/token"] = Token;
  nvs["grok-factory/host"] = "devices.example.net";
  nvs["grok-factory/port"] = "8444";
  nvs["grok-factory/ca"] = Ca;
  nvs["grok-factory/passphrase"] = Passphrase;
  nvs["grok-factory/setup_code"] = Code;
}
// No reply, page or console line ever carries a stored value.
void secretFree(const std::string &text) {
  for (const char *secret : {Token, Passphrase, Code, "pass-word-1", "home-net-wrong"})
    assert(text.find(secret) == std::string::npos);
}

// A standalone hotspot over its own settings, for the window and the code limits.
struct Bench {
  std::map<std::string, std::string> saved;
  grok::NvsStore store;
  grok::WifiSettings settings;
  grok::FactorySettings factory;
  grok::SetupHotspot hotspot{store, settings, factory};
  explicit Bench(uint32_t now) {
    factory.port = 8444;
    strcpy(factory.token, Token);
    strcpy(factory.host, "devices.example.net");
    strcpy(factory.ca, Ca);
    strcpy(factory.passphrase, Passphrase);
    strcpy(factory.setupCode, Code);
    assert(hotspot.begin("oledkit-0123456789ab", now));
  }
};

void guardLimits() {
  grok::SetupGuard guard;
  uint32_t now = 1000;
  using V = grok::SetupGuard::Verdict;
  assert(guard.check(Code, "abcd-2345", now) == V::Ok); // As the label prints it.
  assert(guard.check(Code, " ABCD 2345 ", now) == V::Ok);
  for (int i = 0; i < 4; ++i)
    assert(guard.check(Code, "ABCD2346", now) == V::Wrong);
  assert(!guard.locked(now));
  assert(guard.check(Code, "ABCD234", now) == V::Wrong); // The fifth wrong code locks.
  assert(guard.locked(now) && guard.retryAfterSeconds(now) == 60);
  assert(guard.check(Code, Code, now + 59000) == V::Locked); // Even the right code waits.
  assert(guard.check(Code, Code, now + 60000) == V::Ok);
  assert(guard.check(Code, "WRONG000", now + 60000) == V::Wrong); // Sixth: 2 minutes.
  assert(guard.retryAfterSeconds(now + 60000) == 120);
  now += 60000;
  for (int wrong = 6; wrong < 19; ++wrong) {
    now += 7200000; // Longer than the longest lock (64 minutes).
    assert(guard.check(Code, "WRONG000", now) == V::Wrong);
  }
  now += 7200000; // Longer than the longest lock (64 minutes).
  assert(guard.check(Code, "WRONG000", now) == V::Closed); // The twentieth closes setup.
  assert(guard.closed() && guard.check(Code, Code, now + 3600000) == V::Closed);

  char name[16];
  grok::setupHotspotName("oledkit-0123456789ab", name);
  assert(!strcmp(name, "GrokGadget-89AB"));
  char code[grok::SetupCodeLength + 1];
  assert(!grok::normalizeSetupCode("ABCD23456", code) && !grok::normalizeSetupCode("ABC", code));
}

int main() {
  guardLimits();

  // A board without a factory identity never opens the hotspot.
  {
    grok::NvsStore store;
    grok::WifiSettings none;
    grok::FactorySettings empty;
    grok::SetupHotspot off(store, none, empty);
    assert(!off.begin("oledkit-0123456789ab", 0) && !WiFi.apUp);
  }

  // Window: 10 minutes, then the hotspot closes and stays closed until a power cycle.
  {
    Bench bench(0);
    assert(bench.hotspot.minutesLeft(0) == 10 && bench.hotspot.minutesLeft(540001) == 1);
    bench.hotspot.loop(599999, false);
    assert(bench.hotspot.state() == State::Open && WiFi.apUp);
    bench.hotspot.loop(600000, false);
    assert(bench.hotspot.state() == State::Expired && !WiFi.apUp && !bench.hotspot.active());
  }

  // Factory-provisioned, no home Wi-Fi: the board opens the hotspot at power-on.
  writeFactory(fake::nvs);
  WiFi.scans = 0;
  WiFi.networks = {{"home-net", true}, {"cafe", false}, {"home-net", true}, {"", true}};
  setup();
  advance(50);
  assert(WiFi.apUp && WiFi.apName == "GrokGadget-3412" && WiFi.apPassphrase == Passphrase);
  assert(WiFi.apMaxStations == 1 && WiFi.currentMode == WIFI_AP_STA && WiFi.scans == 1);
  assert(hotspot.state() == State::Open && tls.connects == 0);
  assert(fake::screen[0].rfind("Setup 10 min", 0) == 0);
  assert(fake::screen[1].rfind("GrokGadget-3412", 0) == 0);
  assert(fake::screen[3].rfind("Scan the label", 0) == 0);
  WebServer &page = hotspot.serverForTests();

  // The page and the captive checks of both phones.
  assert(page.request(HTTP_GET, "/") == 200 && page.type == "text/html");
  assert(page.body.find("Setup code from the label") != std::string::npos);
  assert(page.headers["Cache-Control"] == "no-store");
  secretFree(page.body);
  for (auto probe : {"/hotspot-detect.html", "/generate_204", "/connecttest.txt"}) {
    assert(page.request(HTTP_GET, probe) == 302);
    assert(page.headers["Location"] == "http://192.168.4.1/");
  }
  assert(page.request(HTTP_GET, "/status") == 200 &&
         page.body.find("\"open\"") != std::string::npos);
  assert(page.body.find("\"minutes_left\":10") != std::string::npos);

  // The setup code comes first: a wrong code reveals nothing, the label's code lists networks.
  assert(page.request(HTTP_POST, "/unlock", {{"code", "WXYZ6789"}}) == 403);
  assert(page.body == R"({"ok":false,"error":"wrong_code"})");
  assert(page.request(HTTP_POST, "/join", {{"code", "WXYZ6789"}, {"ssid", "home-net"}}) == 403);
  assert(page.request(HTTP_POST, "/unlock", {{"code", "abcd-2345"}}) == 200);
  assert(!deserializeJson(doc, page.body) && doc["ok"] == true);
  assert(doc["networks"].size() == 2 && doc["networks"][0]["name"] == "home-net");
  assert(doc["networks"][1]["secure"] == false);
  secretFree(page.body);

  // A wrong password: the page and the display say so, and nothing is stored.
  assert(page.request(HTTP_POST, "/join",
                      {{"code", Code}, {"ssid", "home-net"}, {"password", "short"}}) == 400);
  assert(page.body.find("invalid_settings") != std::string::npos);
  assert(page.request(HTTP_POST, "/join",
                      {{"code", Code}, {"ssid", "home-net"}, {"password", "home-net-wrong"}}) ==
         202);
  assert(hotspot.state() == State::Joining && WiFi.ssid == "home-net");
  advance(100);
  assert(fake::screen[0].rfind("Joining WiFi", 0) == 0);
  WiFi.fail(15); // 4-way handshake timeout: the usual wrong-password result.
  advance(100);
  assert(hotspot.state() == State::Failed);
  assert(page.request(HTTP_GET, "/status") == 200);
  assert(page.body.find("\"failed\"") != std::string::npos);
  assert(page.body.find("wrong_password") != std::string::npos);
  secretFree(page.body);
  assert(fake::screen[0].rfind("Wrong password", 0) == 0);
  assert(fake::screen[1].rfind("GrokGadget-3412", 0) == 0);
  assert(fake::nvs.count("grok-wifi/ssid") == 0 && fake::nvs.count("grok-wifi/pass") == 0);

  // No answer at all: a timeout after 20 seconds.
  assert(page.request(HTTP_POST, "/join",
                      {{"code", Code}, {"ssid", "far-away"}, {"password", "pass-word-1"}}) == 202);
  advance(grok::SetupJoinMs + 10);
  assert(hotspot.state() == State::Failed && hotspot.failure() == grok::JoinFailure::Timeout);

  // The right password: Wi-Fi is saved, the link starts and the hotspot stays for the page.
  assert(page.request(HTTP_POST, "/join",
                      {{"code", Code}, {"ssid", "home-net"}, {"password", "pass-word-1"}}) == 202);
  WiFi.state = WL_CONNECTED;
  WiFi.dns["devices.example.net"] = IPAddress(203, 0, 113, 7);
  int begins = WiFi.begins;
  advance(100);
  assert(hotspot.state() == State::Joined && WiFi.apUp && WiFi.currentMode == WIFI_AP_STA);
  assert(WiFi.begins == begins); // The link reuses the page's join.
  assert(fake::nvs["grok-wifi/ssid"] == "home-net" && fake::nvs["grok-wifi/pass"] == "pass-word-1");
  assert(fake::nvs.count("grok-wifi/token") == 0 && settings.complete());
  assert(page.request(HTTP_GET, "/status") == 200 &&
         page.body.find("\"joined\"") != std::string::npos);
  // After the join the page refuses new Wi-Fi details.
  assert(page.request(HTTP_POST, "/join", {{"code", Code}, {"ssid", "x"}, {"password", ""}}) ==
         409);

  // The relay answers hello: Online on the page and the display, then the hotspot closes.
  for (int i = 0; i < 6000 && tls.output.find('\n') == std::string::npos; ++i)
    advance(1);
  assert(tls.host == "devices.example.net" && tls.port == 8444 && tls.ca == Ca);
  assert(tls.output.find(Token) != std::string::npos); // hello carries the factory token
  tls.output.clear();
  tls.input += R"({"ok":true,"session_id":"s1","protocol_version":"0.1.0"})"
               "\n";
  advance(50);
  assert(gadget.connected() && hotspot.state() == State::Online);
  assert(page.request(HTTP_GET, "/status") == 200 &&
         page.body.find("\"online\"") != std::string::npos);
  assert(fake::screen[0].rfind("Online", 0) == 0);
  advance(grok::SetupOnlineGraceMs);
  assert(!WiFi.apUp && WiFi.currentMode == WIFI_STA && !hotspot.active());
  assert(page.request(HTTP_GET, "/") == 404);

  // The console reports setup state, never a value.
  Serial.output.clear();
  Serial.input += "{\"type\":\"status\"}\n";
  advance(5);
  assert(Serial.output.find("\"setup\":\"online\"") != std::string::npos);
  assert(Serial.output.find("\"factory\":true") != std::string::npos);
  secretFree(Serial.output);
  std::cout << "Wi-Fi setup tests passed\n";
}
