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
int logoPixels() {
  assert(fake::displayTiles.size() == 36); // 48x48: tile columns 10-15, rows 0-5.
  int lit = 0;
  for (const auto &entry : fake::displayTiles) {
    assert(entry.first.first >= 10 && entry.first.first <= 15);
    assert(entry.first.second >= 0 && entry.first.second <= 5);
    for (uint8_t column : entry.second)
      for (int bit = 0; bit < 8; ++bit)
        lit += (column >> bit) & 1;
  }
  assert(lit > 1300 && lit < 1700); // A 46-pixel disc with eye cutouts, inside 2304 pixels.
  return lit;
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
  assert(fake::tileWrites == 0);

  // Join failures (#73). Reason codes map to a cause; anything else stays unclassified.
  using grok::JoinFailure;
  assert(grok::classifyDisconnect(201) == JoinFailure::NoNetwork);
  for (uint8_t reason : {15, 202, 204})
    assert(grok::classifyDisconnect(reason) == JoinFailure::WrongPassword);
  for (uint8_t reason : {0, 2, 8, 200})
    assert(grok::classifyDisconnect(reason) == JoinFailure::None);
  auto blank = [](const std::string &row) {
    return row.find_first_not_of(' ') == std::string::npos;
  };
  assert(blank(fake::screen[1]));
  // A whole join attempt with no recognised reason: "Join timeout".
  for (uint32_t start = fake::now; fake::now - start < grok::WifiRejoinMs + 6000;)
    advance(1);
  assert(fake::screen[0].rfind("Join timeout", 0) == 0);
  assert(fake::screen[1].rfind("Check router", 0) == 0);
  // Name not found, which includes a 5 GHz-only network: status, hint row and console line.
  Serial.output.clear();
  WiFi.fail(201);
  advance(6000);
  assert(fake::screen[0].rfind("No network", 0) == 0);
  assert(fake::screen[1].rfind("2.4GHz? Name?", 0) == 0);
  assert(Serial.output.find("status: No network. Network not found. Use a 2.4 GHz network") !=
         std::string::npos);
  // Wrong password.
  WiFi.fail(15);
  advance(6000);
  assert(fake::screen[0].rfind("Wrong password", 0) == 0);
  assert(fake::screen[1].rfind("Check password", 0) == 0);
  assert(Serial.output.find("status: Wrong password. ") != std::string::npos);
  // The console status reply names the cause, never the network or password.
  Serial.output.clear();
  Serial.input += "{\"type\":\"status\"}\n";
  loop();
  std::string console = Serial.output.substr(Serial.output.find('{'));
  assert(!deserializeJson(frame, console) && frame["wifi"] == "disconnected");
  assert(frame["wifi_failure"] == "wrong_password");
  assert(std::string(frame["wifi_hint"].as<const char *>()).find("Provision") != std::string::npos);
  assert(console.find("home-net") == std::string::npos);
  assert(console.find("pass-word-1") == std::string::npos);

  // Wi-Fi up: TLS to the provisioned address; hello carries the token.
  WiFi.state = WL_CONNECTED;
  online();
  assert(tls.host == "192.168.1.20" && tls.port == 8767);
  // Joining replaces the failure hint with the Grok Bot label.
  assert(fake::screen[1].rfind("Grok Bot", 0) == 0);
  assert(frame["device"]["device_id"] == "oledkit-bc9a78563412");
  assert(frame["device"]["firmware_version"] == FirmwareVersion);
  advance(10);
  assert(fake::screen[0].rfind("Online", 0) == 0);
  logoPixels();
  const auto firstLogo = fake::displayTiles;
  const auto onlineRows = fake::screen;
  int writes = fake::tileWrites, strings = fake::textDraws, clears = fake::displayClears;
  Serial.output.clear();
  advance(50); // Each iteration also includes the session's existing 1 ms delay.
  assert(fake::tileWrites == writes); // No writes between frame deadlines.
  advance(250);
  logoPixels();
  assert(fake::displayTiles != firstLogo); // The face moves while the link stays online.
  assert(fake::tileWrites > writes && fake::tileWrites <= writes + 5 * oledkit::OnlineLogo::Rows);
  assert(fake::screen == onlineRows && fake::textDraws == strings);
  assert(fake::displayClears == clears && Serial.output.empty());

  // Text and clear; the status row stays.
  untilPoll();
  answer(R"({"ok":true,"commands":[{"command_id":"c1","capability":"display.text",)"
         R"("arguments":{"text":"Hello over Wi-Fi"}}]})");
  assert(next() == "ack" && frame["status"] == "executed");
  answer(R"({"ok":true})");
  assert(fake::screen[6] == "Hello over Wi-Fi" && !fake::screen.count(2));
  assert(fake::screen[0].rfind("Online", 0) == 0);
  logoPixels();
  writes = fake::tileWrites;
  strings = fake::textDraws;
  clears = fake::displayClears;
  advance(450);
  assert(fake::tileWrites > writes && fake::screen[6] == "Hello over Wi-Fi");
  assert(fake::textDraws == strings && fake::displayClears == clears);
  untilPoll();
  answer(R"({"ok":true,"commands":[{"command_id":"c2","capability":"display.clear",)"
         R"("arguments":{}}]})");
  assert(next() == "ack" && frame["status"] == "executed");
  answer(R"({"ok":true})");
  assert(!fake::screen.count(2) && fake::screen[0].rfind("Online", 0) == 0);
  logoPixels(); // A display.clear redraw restores the current logo frame.

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
  writes = fake::tileWrites;
  WiFi.state = WL_DISCONNECTED;
  advance(5);
  assert(!gadget.connected() && !tls.open);
  assert(fake::displayTiles.empty()); // The failure status erases the whole logo area.
  advance(2000);
  assert(fake::screen[0].rfind("WiFi...", 0) == 0);
  assert(fake::tileWrites == writes && fake::screen[2] == "Before the drop");
  // A long outage: the firmware starts a fresh join itself, without waiting for the core.
  int joins = WiFi.begins;
  for (uint32_t start = fake::now; fake::now - start < 3 * grok::WifiRejoinMs + 5000;)
    advance(1);
  assert(WiFi.begins >= joins + 2 && WiFi.disconnects >= 2);
  assert(WiFi.ssid == "home-net" && WiFi.password == "pass-word-1");
  tls.output.clear();
  WiFi.state = WL_CONNECTED;
  online(); // A fresh hello; the old ACK is never resent into the new session.
  assert(fake::displayTiles == firstLogo); // Reconnection starts a fresh animation cycle.
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
  assert(fake::displayTiles.empty());
  tls.accept = true;
  tls.output.clear();
  online();

  // Revoked: "Token rejected" and the next hello waits 60 seconds.
  untilPoll();
  answer(R"({"ok":false,"error":{"code":"revoked","message":"Device credential revoked"}})");
  advance(5);
  assert(!gadget.connected() && gadget.rejected());
  assert(fake::screen[0].rfind("Token rejected", 0) == 0);
  assert(fake::displayTiles.empty());
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
  assert(fake::displayTiles.empty());
  writes = fake::tileWrites;
  holdFor(2000);
  assert(fake::screen[0].rfind("Reset in 5s", 0) == 0);
  assert(fake::tileWrites == writes);
  assert(Serial.output.find("status: Reset in 7s") != std::string::npos);
  fake::pin = 1;
  holdFor(200);
  assert(fake::screen[0].rfind("Online", 0) == 0);
  logoPixels();
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

  // Every frame fits inside its area, a blink changes the eyes, and the cycle repeats. Only
  // frames that look different are drawn.
  oledkit::OnlineLogo clock;
  clock.reset(0);
  clock.draw(*oled);
  const auto neutral = fake::displayTiles;
  const int openEyes = logoPixels();
  bool moved = false, blinked = false;
  int draws = 0;
  for (uint32_t time = 150; time <= 3600; time += 150)
    if (clock.advance(time)) {
      clock.draw(*oled);
      ++draws;
      blinked = blinked || logoPixels() > openEyes;
      moved = moved || fake::displayTiles != neutral;
    }
  assert(moved && blinked && draws < 24 && fake::displayTiles == neutral);

  // The frame clock survives millis() wraparound and catches up without replaying frames.
  clock.reset(UINT32_MAX - 99);
  assert(!clock.advance(49));
  assert(clock.advance(50));
  assert(!clock.advance(50));
  assert(clock.advance(86400050));
  assert(!clock.advance(86400050));
  // Under the Online face, text uses rows 6 and 7: by word when two lines suffice,
  // otherwise by character. All 32 characters stay visible.
  strcpy(shownStatus, "Online");
  strcpy(text, "aaaaaaa bbbbbbb ccccccc ddddddd");
  draw();
  assert(fake::screen[6] == "aaaaaaa bbbbbbb" && fake::screen[7] == "ccccccc ddddddd");
  assert(fake::displayTiles.size() == 36);
  strcpy(text, "Grok Gadgets says hello 12345678"); // Three lines by word: falls back.
  draw();
  assert(fake::screen[6] == "Grok Gadgets say" && fake::screen[7] == "s hello 12345678");
  strcpy(text, "abcdefghijklmnopqrstuvwxyz012345");
  draw();
  assert(fake::screen[6] == "abcdefghijklmnop" && fake::screen[7] == "qrstuvwxyz012345");
  // Without the face, text keeps all 16 columns and no face pixels remain.
  strcpy(shownStatus, "WiFi...");
  draw();
  assert(fake::screen[2] == "abcdefghijklmnop" && fake::screen[4] == "qrstuvwxyz012345");
  assert(!fake::screen.count(6) && fake::displayTiles.empty());
  std::cout << "Wi-Fi link tests passed\n";
}
