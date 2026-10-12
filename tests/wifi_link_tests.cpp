// SPDX-License-Identifier: Apache-2.0
// Wi-Fi build of the OLED kit, provisioned: TLS link lifecycle against a scripted gateway.
#define OLED_KIT_WIFI
#include "../examples/oled-kit/main.cpp"
#include "host_board.h"
#include <cassert>
#include <iostream>

const char *Ca =
    "-----BEGIN CERTIFICATE-----\nMIIBpinnedTestCertificateBodyOnlyForHostTests0123456789abcdef\n"
    "-----END CERTIFICATE-----\n";
const char *Token = "device-token-0123456789";
WiFiClientSecure &tls = static_cast<WiFiClientSecure &>(wifiLink.stream());
DynamicJsonDocument frame(4096);

void advance(uint32_t ms) {
  for (uint32_t i = 0; i < ms; ++i) {
    fake::now += 1;
    loop();
  }
}
// The next frame the gadget wrote to the TLS link, waiting up to `ms`.
std::string next(uint32_t ms = 1000) {
  for (uint32_t i = 0; i < ms && tls.output.find('\n') == std::string::npos; ++i)
    advance(1);
  size_t end = tls.output.find('\n');
  assert(end != std::string::npos);
  std::string line = tls.output.substr(0, end);
  tls.output.erase(0, end + 1);
  assert(!deserializeJson(frame, line));
  return frame["type"].as<std::string>();
}
void answer(const std::string &json) {
  tls.input += json + "\n";
  loop();
}
const char *helloOk = R"({"ok":true,"session_id":"s1","protocol_version":"0.1.0"})";
// Answer state frames until the next poll and return with that poll unanswered.
void untilPoll() {
  std::string type;
  while ((type = next()) == "state")
    answer(R"({"ok":true})");
  assert(type == "poll");
}
void online() {
  assert(next(6000) == "hello");
  assert(frame["token"] == Token);
  answer(helloOk);
  assert(gadget.connected());
}

int main() {
  fake::nvs = {{"grok-wifi/ssid", "home-net"}, {"grok-wifi/pass", "pass-word-1"},
               {"grok-wifi/token", Token},     {"grok-wifi/host", "192.168.1.20"},
               {"grok-wifi/port", "8767"},     {"grok-wifi/ca", Ca}};
  setup();
  assert(WiFi.begins == 1 && WiFi.ssid == "home-net" && WiFi.password == "pass-word-1");
  assert(tls.ca == Ca); // The pinned CA, never a bundle or insecure mode.

  // No Wi-Fi yet: no TLS attempt, status "WiFi...".
  advance(3000);
  assert(tls.connects == 0 && fake::screen[0].rfind("WiFi...", 0) == 0);

  // Wi-Fi up: TLS to the provisioned address; hello carries the token.
  WiFi.state = WL_CONNECTED;
  online();
  assert(tls.host == "192.168.1.20" && tls.port == 8767);
  assert(frame["device"]["device_id"] == "oledkit-bc9a78563412");
  assert(frame["device"]["firmware_version"] == "0.2.0-wifi");
  advance(10);
  assert(fake::screen[0].rfind("Online", 0) == 0);

  // Text and clear; the status row stays.
  untilPoll();
  answer(R"({"ok":true,"commands":[{"command_id":"c1","capability":"display.text",)"
         R"("arguments":{"text":"Hello over Wi-Fi"}}]})");
  assert(next() == "ack" && frame["status"] == "executed");
  answer(R"({"ok":true})");
  assert(fake::screen[2] == "Hello over Wi-Fi" && fake::screen[0].rfind("Online", 0) == 0);
  untilPoll();
  answer(R"({"ok":true,"commands":[{"command_id":"c2","capability":"display.clear",)"
         R"("arguments":{}}]})");
  assert(next() == "ack" && frame["status"] == "executed");
  answer(R"({"ok":true})");
  assert(!fake::screen.count(2) && fake::screen[0].rfind("Online", 0) == 0);

  // BOOT press and release become button events over Wi-Fi.
  fake::pin = 0;
  std::string type;
  while ((type = next()) != "event")
    answer(type == "poll" ? R"({"ok":true,"commands":[]})" : R"({"ok":true})");
  assert(frame["name"] == "button" && frame["data"]["pressed"] == true);
  answer(R"({"ok":true})");
  fake::pin = 1;
  while ((type = next()) != "event")
    answer(type == "poll" ? R"({"ok":true,"commands":[]})" : R"({"ok":true})");
  assert(frame["data"]["pressed"] == false);
  answer(R"({"ok":true})");

  // Wi-Fi drops while a command is outstanding: the link closes, nothing is replayed.
  untilPoll();
  answer(R"({"ok":true,"commands":[{"command_id":"c3","capability":"display.text",)"
         R"("arguments":{"text":"Before the drop"}}]})");
  assert(next() == "ack");
  WiFi.state = WL_DISCONNECTED;
  advance(5);
  assert(!gadget.connected() && !tls.open);
  advance(2000);
  assert(fake::screen[0].rfind("WiFi...", 0) == 0);
  // A long outage: the firmware starts a fresh join itself, without waiting for the core.
  int joins = WiFi.begins;
  for (uint32_t start = fake::now; fake::now - start < 3 * grok::WifiRejoinMs + 5000;)
    advance(1);
  assert(WiFi.begins >= joins + 2 && WiFi.disconnects >= 2);
  assert(WiFi.ssid == "home-net" && WiFi.password == "pass-word-1");
  tls.output.clear();
  WiFi.state = WL_CONNECTED;
  online(); // A fresh hello; the old ACK is never resent into the new session.
  untilPoll();
  answer(R"({"ok":true,"commands":[]})");

  // The gateway restarts: the socket closes and the gadget reconnects with backoff.
  tls.open = false;
  advance(5);
  assert(!gadget.connected());
  tls.output.clear();
  online();

  // Wrong CA: the handshake fails verification; the status says so and it keeps retrying.
  tls.open = false;
  tls.accept = false;
  tls.error = grok::TlsVerifyFailed;
  int before = tls.connects;
  advance(12000);
  assert(tls.connects > before + 1 && fake::screen[0].rfind("TLS rejected", 0) == 0);
  tls.accept = true;
  tls.output.clear();
  online();

  // Revoked: "Token rejected" and the next hello waits 60 seconds.
  untilPoll();
  answer(R"({"ok":false,"error":{"code":"revoked","message":"Device credential revoked"}})");
  advance(5);
  assert(!gadget.connected() && gadget.rejected());
  assert(fake::screen[0].rfind("Token rejected", 0) == 0);
  before = tls.connects;
  for (uint32_t start = fake::now; fake::now - start < 59000;)
    advance(1);
  assert(tls.connects == before);
  tls.output.clear();
  online(); // A re-enrolled token would be provisioned over USB; here the gateway accepts.
  assert(!gadget.rejected());

  // BOOT held past 3 seconds: the status row counts down. Letting go keeps the settings.
  auto holdFor = [&](uint32_t ms) {
    for (uint32_t start = fake::now; fake::now - start < ms;) {
      advance(1);
      if (tls.output.find('\n') != std::string::npos) {
        std::string kind = next(1);
        answer(kind == "poll" ? R"({"ok":true,"commands":[]})" : R"({"ok":true})");
      }
    }
  };
  Serial.output.clear();
  fake::pin = 0;
  holdFor(2900);
  assert(fake::screen[0].rfind("Online", 0) == 0);
  holdFor(200);
  assert(fake::screen[0].rfind("Reset in 7s", 0) == 0);
  holdFor(2000);
  assert(fake::screen[0].rfind("Reset in 5s", 0) == 0);
  assert(Serial.output.find("status: Reset in 7s") != std::string::npos);
  fake::pin = 1;
  holdFor(200);
  assert(fake::screen[0].rfind("Online", 0) == 0);
  assert(ESP.restarts == 0 && fake::nvs.count("grok-wifi/token"));

  // BOOT held for 10 seconds: factory reset and restart.
  fake::pin = 0;
  for (int i = 0; i < 10100 && !ESP.restarts; ++i) {
    advance(1);
    if (tls.output.find('\n') != std::string::npos) {
      type = next(1);
      answer(type == "poll" ? R"({"ok":true,"commands":[]})" : R"({"ok":true})");
    }
  }
  assert(ESP.restarts == 1 && fake::nvs.empty());
  std::cout << "Wi-Fi link tests passed\n";
}
