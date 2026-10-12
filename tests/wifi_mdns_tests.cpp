// SPDX-License-Identifier: Apache-2.0
// Wi-Fi build with the gateway provisioned as an mDNS name (hub ADR 006).
#define OLED_KIT_WIFI
#include "../examples/oled-kit/main.cpp"
#include "host_board.h"
#include <cassert>
#include <iostream>

const char *Ca =
    "-----BEGIN CERTIFICATE-----\nMIIBnameTestCertificateBodyOnlyForHostTests0123456789\n"
    "-----END CERTIFICATE-----\n";
WiFiClientSecure &tls = static_cast<WiFiClientSecure &>(wifiLink.stream());

void advance(uint32_t ms) {
  for (uint32_t i = 0; i < ms; ++i) {
    fake::now += 1;
    loop();
  }
}

int main() {
  fake::nvs = {{"grok-wifi/ssid", "home-net"},
               {"grok-wifi/pass", "pass-word-1"},
               {"grok-wifi/token", "device-token-0123456789"},
               {"grok-wifi/host", "my-mac.local"},
               {"grok-wifi/port", "8767"},
               {"grok-wifi/ca", Ca}};
  setup();
  WiFi.state = WL_CONNECTED;

  // The name does not answer: no TLS attempt, and the status says so.
  advance(3000);
  assert(MDNS.started && MDNS.name == "oledkit-bc9a78563412" && MDNS.queries > 0);
  assert(tls.connects == 0 && fake::screen[0].rfind("Gateway name?", 0) == 0);

  // The name answers: connect to its address and check the certificate against the name.
  MDNS.hosts["my-mac"] = IPAddress(192, 168, 1, 20);
  advance(6000);
  assert(tls.connects > 0 && tls.address == "192.168.1.20" && tls.host == "my-mac.local");
  assert(tls.port == 8767 && tls.ca == Ca);

  // The computer moves to a new address: the next connection follows the name.
  tls.open = false;
  tls.output.clear();
  MDNS.hosts["my-mac"] = IPAddress(192, 168, 1, 42);
  advance(6000);
  assert(tls.address == "192.168.1.42" && tls.host == "my-mac.local");
  std::cout << "Wi-Fi mDNS tests passed\n";
}
