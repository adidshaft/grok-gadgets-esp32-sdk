// SPDX-License-Identifier: Apache-2.0
// grok::WifiApp (hub ADR 013) through the switch example: phone setup, the claim code on the
// setup page for a gadget without a display (ADR 011 §10), and the switch's safe-off.
#define GROK_HOST_SIMULATION
#define GROK_SETUP_TESTS
#include "../examples/switch/main.cpp"
#include "host_board.h"
#include <cassert>
#include <iostream>

const char *Ca = "-----BEGIN CERTIFICATE-----\nMIIBappTestCertificateBodyOnly0123456789ab\n"
                 "-----END CERTIFICATE-----\n";
const char *Code = "ABCD2345";
WiFiClientSecure &tls = static_cast<WiFiClientSecure &>(app.linkForTests().stream());

void advance(uint32_t ms) {
  for (uint32_t i = 0; i < ms; ++i) {
    fake::now += 1;
    loop();
  }
}
void reply(const char *json) {
  tls.input += json;
  tls.input += "\n";
}
std::string nextFrame() {
  for (int i = 0; i < 2000 && tls.output.find('\n') == std::string::npos; ++i)
    advance(1);
  size_t end = tls.output.find('\n');
  assert(end != std::string::npos);
  std::string line = tls.output.substr(0, end);
  tls.output.erase(0, end + 1);
  return line;
}

int main() {
  fake::nvs["grok-factory/token"] = "device-token-0123456789";
  fake::nvs["grok-factory/host"] = "devices.example.net";
  fake::nvs["grok-factory/port"] = "8444";
  fake::nvs["grok-factory/ca"] = Ca;
  fake::nvs["grok-factory/passphrase"] = "K7QXM2PAHJ4R9TVW";
  fake::nvs["grok-factory/setup_code"] = Code;
  WiFi.networks = {{"home-net", true}};
  setup();
  assert(fake::led == HIGH && !on); // Off at boot: most modules switch on when LOW.
  advance(50);
  using State = grok::SetupHotspot::State;
  auto &hotspot = app.hotspotForTests();
  WebServer &page = hotspot.serverForTests();
  assert(hotspot.state() == State::Open && strncmp(app.status(), "Setup", 5) == 0);

  // The phone joins the gadget to home Wi-Fi.
  assert(page.request(HTTP_POST, "/unlock", {{"code", Code}}) == 200);
  assert(page.request(HTTP_POST, "/join",
                      {{"code", Code}, {"ssid", "home-net"}, {"password", "pass-word-1"}}) == 202);
  WiFi.state = WL_CONNECTED;
  WiFi.dns["devices.example.net"] = IPAddress(203, 0, 113, 7);
  advance(100);
  assert(hotspot.state() == State::Joined);

  // The relay answers with a claim code: the page shows it, the console never does.
  std::string hello = nextFrame();
  assert(hello.find("\"claim\":\"after_setup\"") != std::string::npos);
  reply(R"({"ok":true,"session_id":"s1","protocol_version":"0.1.0","claimed":false,)"
        R"("claim_code":"JKLM-NPQR"})");
  advance(20);
  assert(app.online() && hotspot.state() == State::Online);
  assert(page.request(HTTP_GET, "/status") == 200);
  assert(page.body.find("\"claim_code\":\"JKLM-NPQR\"") != std::string::npos);
  assert(page.request(HTTP_GET, "/") == 200 &&
         page.body.find("Claim my gadget with code") != std::string::npos);
  assert(Serial.output.find("JKLM-NPQR") == std::string::npos);
  assert(Serial.output.find("Code shown on the setup page") != std::string::npos);
  // With a code to read, the hotspot stays longer than the usual 30 seconds.
  for (int i = 0; i < 60; ++i) {
    advance(1000);
    while (tls.output.find('\n') != std::string::npos) {
      std::string frame = nextFrame();
      if (frame.find("\"poll\"") != std::string::npos)
        reply(R"({"ok":true,"commands":[]})");
      else
        reply(R"({"ok":true})");
    }
  }
  assert(hotspot.active());

  // switch.set turns the load on; a link loss turns it off after 30 seconds.
  std::string frame;
  while ((frame = nextFrame()).find("\"poll\"") == std::string::npos)
    reply(R"({"ok":true})");
  reply(R"({"ok":true,"commands":[{"command_id":"c1","capability":"switch.set",)"
        R"("arguments":{"on":true}}]})");
  frame = nextFrame();
  assert(frame.find("\"ack\"") != std::string::npos &&
         frame.find("\"executed\"") != std::string::npos);
  assert(on && fake::led == LOW);
  reply(R"({"ok":true})");
  advance(10);
  tls.accept = false; // The relay link drops and stays down.
  tls.open = false;
  uint32_t dropped = fake::now;
  // Each loop also sleeps 1 ms, so run until a clock time rather than for a count.
  while (fake::now < dropped + 29000)
    advance(1);
  assert(on);
  while (fake::now < dropped + 31000)
    advance(1);
  assert(!on && fake::led == HIGH && offByLinkLoss);
  std::cout << "app tests passed\n";
}
