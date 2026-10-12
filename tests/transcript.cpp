// SPDX-License-Identifier: Apache-2.0
// Prints every frame an example sketch writes while a scripted gateway answers it.
// Arguments are command objects delivered one per poll. tools/check_contract.py validates
// the output against the pinned schema. GROK_WIFI_TRANSCRIPT runs the Wi-Fi build of the OLED kit,
// provisioned with a test token, and scripts the gateway on its TLS link instead of USB serial.
#define GROK_HOST_SIMULATION
#ifdef GROK_WIFI_TRANSCRIPT
#define OLED_KIT_WIFI
#endif
#include "host_board.h"
#include <iostream>
#include <string>
#include <vector>
#include GROK_EXAMPLE

#ifdef GROK_WIFI_TRANSCRIPT
std::string &wireOut() { return static_cast<WiFiClientSecure &>(wifiLink.stream()).output; }
std::string &wireIn() { return static_cast<WiFiClientSecure &>(wifiLink.stream()).input; }
#else
std::string &wireOut() { return Serial.output; }
std::string &wireIn() { return Serial.input; }
#endif

namespace script {
std::vector<std::string> commands;
bool holding = false;
std::string held;

std::string respond(const std::string &type) {
  if (type == "hello")
    return R"({"ok":true,"session_id":"transcript","protocol_version":"0.1.0"})";
  if (type == "poll") {
    if (commands.empty())
      return R"({"ok":true,"commands":[]})";
    auto command = commands.front();
    commands.erase(commands.begin());
    return R"({"ok":true,"commands":[)" + command + "]}";
  }
  return R"({"ok":true})";
}
void step(uint32_t ms) {
  for (uint32_t i = 0; i < ms; ++i) {
    loop();
    size_t end;
    while ((end = wireOut().find('\n')) != std::string::npos) {
      std::string line = wireOut().substr(0, end);
      wireOut().erase(0, end + 1);
      std::cout << line << '\n';
      StaticJsonDocument<256> filter;
      filter["type"] = true;
      StaticJsonDocument<256> frame;
      deserializeJson(frame, line, DeserializationOption::Filter(filter));
      std::string answer = respond(frame["type"] | "");
      if (holding)
        held = answer;
      else
        wireIn() += answer + "\n";
    }
  }
}
} // namespace script
int main(int argc, char **argv) {
  using script::holding;
  using script::step;
  script::commands.assign(argv + 1, argv + argc);
#ifdef GROK_WIFI_TRANSCRIPT
  fake::nvs = {
      {"grok-wifi/ssid", "transcript-net"},
      {"grok-wifi/pass", "transcript-pass"},
      {"grok-wifi/token", "transcript-device-token"},
      {"grok-wifi/host", "192.168.1.20"},
      {"grok-wifi/port", "8767"},
      {"grok-wifi/ca",
       "-----BEGIN CERTIFICATE-----\nMIIBtranscriptOnlyNotARealCertificate0123456789abcdef\n"
       "-----END CERTIFICATE-----\n"}};
  WiFi.state = WL_CONNECTED;
#endif
  setup();
  step(1000 + 400 * argc);
  fake::pin = 0; // Press and hold, then release.
  step(1500);
  fake::pin = 1;
  step(500);
  holding = true; // Stall one reply while 20 edges overflow the event queue.
  step(5);
  for (int i = 0; i < 20; ++i) {
    fake::pin = i % 2;
    step(40);
  }
  fake::pin = 1;
  holding = false;
  wireIn() += script::held + "\n";
  step(3000);
  step(4000);
}
