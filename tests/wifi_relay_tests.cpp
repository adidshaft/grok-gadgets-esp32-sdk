// SPDX-License-Identifier: Apache-2.0
// Wi-Fi build with the gateway provisioned as a relay's DNS name (hub ADR 009).
#define OLED_KIT_WIFI
#include "../examples/oled-kit/main.cpp"
#include "host_board.h"
#include <cassert>
#include <iostream>

const char *Ca =
    "-----BEGIN CERTIFICATE-----\nMIIBrelayTestCertificateBodyOnlyForHostTests0123456789\n"
    "-----END CERTIFICATE-----\n";
WiFiClientSecure &tls = static_cast<WiFiClientSecure &>(wifiLink.stream());

void advance(uint32_t ms) {
  for (uint32_t i = 0; i < ms; ++i) {
    fake::now += 1;
    loop();
  }
}

void names() {
  for (auto good : {"devices.example.net", "a.co", "relay-1.home.example", "x1.example.net"})
    assert(grok::validDnsName(good));
  for (auto bad : {"", "gateway", "my-mac.local", "Devices.example.net", "a..b.net",
                   "-a.example.net", "a-.example.net", "relay.example.123", "relay.example.net.",
                   ".example.net", "relay_1.example.net", "192.168.1.20"})
    assert(!grok::validDnsName(bad));
  // The reconnect wait stays between MinBackoffMs and the current backoff.
  for (uint32_t backoff : {grok::MinBackoffMs, 1000u, 2000u, 4000u, grok::MaxBackoffMs})
    for (uint32_t random : {0u, 1u, 77u, 1000u, 0xffffffffu}) {
      uint32_t wait = grok::jitteredBackoff(backoff, random);
      assert(wait >= grok::MinBackoffMs && wait <= backoff && wait >= backoff * 3 / 4);
    }
  assert(grok::jitteredBackoff(4000, 0) != grok::jitteredBackoff(4000, 500));
}

int main() {
  names();
  fake::nvs = {{"grok-wifi/ssid", "home-net"},
               {"grok-wifi/pass", "pass-word-1"},
               {"grok-wifi/token", "device-token-0123456789"},
               {"grok-wifi/host", "devices.example.net"},
               {"grok-wifi/port", "8443"},
               {"grok-wifi/ca", Ca}};
  setup();
  assert(wifiLink.relay());
  WiFi.state = WL_CONNECTED;

  // The name does not resolve: no TLS attempt, no mDNS, and the status says so.
  advance(3000);
  assert(WiFi.lookups > 0 && tls.connects == 0 && MDNS.queries == 0);
  assert(fake::screen[0].rfind("Relay name?", 0) == 0);

  // The relay refuses or is down: the status shows the relay is being reached.
  WiFi.dns["devices.example.net"] = IPAddress(203, 0, 113, 7);
  tls.accept = false;
  advance(6000);
  assert(tls.connects > 0 && fake::screen[0].rfind("Relay...", 0) == 0);

  // The name resolves: connect to its address and check the certificate against the name.
  tls.accept = true;
  advance(6000);
  assert(tls.address == "203.0.113.7" && tls.host == "devices.example.net");
  assert(tls.port == 8443 && tls.ca == Ca);

  // The pinned CA does not match: the status says so instead of retrying silently.
  tls.open = false;
  tls.accept = false;
  tls.error = grok::TlsVerifyFailed;
  advance(6000);
  assert(fake::screen[0].rfind("TLS rejected", 0) == 0);
  std::cout << "Wi-Fi relay tests passed\n";
}
