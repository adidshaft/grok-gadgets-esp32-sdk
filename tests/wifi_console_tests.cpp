// SPDX-License-Identifier: Apache-2.0
// Wi-Fi build of the OLED kit, unprovisioned: the USB serial provisioning console.
#define OLED_KIT_WIFI
#include "../examples/oled-kit/main.cpp"
#include "host_board.h"
#include <cassert>
#include <iostream>

const char *Ca =
    "-----BEGIN CERTIFICATE-----\nMIIBtestCertificateBodyOnlyForHostTests0123456789abcdef\n"
    "-----END CERTIFICATE-----\n";
DynamicJsonDocument reply(1024);

// Send one console line; return the JSON reply and drop any `status:` text lines.
void command(const std::string &line) {
  Serial.input += line + "\n";
  loop();
  std::string json;
  size_t start = 0, end;
  while ((end = Serial.output.find('\n', start)) != std::string::npos) {
    std::string text = Serial.output.substr(start, end - start);
    if (!text.empty() && text[0] == '{')
      json = text;
    start = end + 1;
  }
  Serial.output.clear();
  assert(!json.empty() && !deserializeJson(reply, json));
}
std::string provision(const char *ssid, const char *password, const char *token,
                      const char *host = "192.168.1.20", int port = 8767, const char *ca = Ca) {
  DynamicJsonDocument doc(2048);
  doc["type"] = "provision";
  doc["ssid"] = ssid;
  doc["password"] = password;
  doc["token"] = token;
  doc["host"] = host;
  doc["port"] = port;
  doc["ca"] = ca;
  std::string line;
  serializeJson(doc, line);
  return line;
}
void secretFree(const std::string &text) {
  for (const char *secret : {"home-net", "pass-word-1", "device-token-0123456789"})
    assert(text.find(secret) == std::string::npos);
}

int main() {
  setup();
  for (int i = 0; i < 50; ++i) {
    fake::now += 10;
    loop();
  }
  assert(fake::screen[0].rfind("Not provisioned", 0) == 0);
  assert(fake::tileWrites == 0); // The Online logo never runs before provisioning.
  assert(Serial.output.find("status: Not provisioned") != std::string::npos);
  Serial.output.clear();
  assert(Serial.rxBuffer >= grok::MaxConsoleLine);

  command(R"({"type":"status"})");
  assert(reply["ok"] && reply["device_id"] == "oledkit-bc9a78563412");
  assert(reply["provisioned"] == false && reply["gateway"] == "Not provisioned");
  assert(reply["firmware"] == "0.2.0-wifi");

  // Invalid settings are refused without echoing any value, and nothing is stored.
  const char *token = "device-token-0123456789";
  for (auto line :
       {provision("home-net", "short", token), provision("", "pass-word-1", token),
        provision("home-net", "pass-word-1", "short"),
        provision("home-net", "pass-word-1", "has space in token xx"),
        provision("home-net", "pass-word-1", token, "gateway.lan"),
        provision("home-net", "pass-word-1", token, "my_mac.local"),
        provision("home-net", "pass-word-1", token, "-mac.local"),
        provision("home-net", "pass-word-1", token, ".local"),
        provision("home-net", "pass-word-1", token, "192.168.1.256"),
        provision("home-net", "pass-word-1", token, "192.168.1.20", 0),
        provision("home-net", "pass-word-1", token, "192.168.1.20", 8767, "not a pem")}) {
    Serial.output.clear();
    command(line);
    assert(reply["ok"] == false && reply["error"]["code"] == "invalid_settings");
    std::string json;
    serializeJson(reply, json);
    secretFree(json);
  }
  // A rejected line leaves no partial value behind in the console's settings buffer.
  {
    DynamicJsonDocument doc(2048);
    assert(!deserializeJson(doc, provision("home-net", "pass-word-1", "short")));
    grok::WifiSettings scratch;
    assert(grok::readSettingsInto(doc.as<JsonObjectConst>(), scratch).code);
    assert(!scratch.ssid[0] && !scratch.password[0] && !scratch.token[0]);
  }
  command(R"({"type":"provision","ssid":"home-net"})");
  assert(reply["error"]["code"] == "invalid_settings");
  assert(fake::nvs.empty() && ESP.restarts == 0);
  command(R"({"type":"reboot"})");
  assert(reply["error"]["code"] == "unknown_command");

  // An overlong line is refused; a log line from a terminal is ignored.
  Serial.input += std::string(grok::MaxConsoleLine + 10, 'x');
  command("");
  assert(reply["error"]["code"] == "invalid_request");
  Serial.input += "not json\n";
  loop();
  assert(Serial.output.empty());

  // Valid settings are stored, the reply carries no value, and the board restarts.
  command(provision("home-net", "pass-word-1", token));
  assert(reply["ok"] && reply["restarting"] && reply["device_id"] == "oledkit-bc9a78563412");
  std::string json;
  serializeJson(reply, json);
  secretFree(json);
  assert(fake::nvs["grok-wifi/ssid"] == "home-net" && fake::nvs["grok-wifi/pass"] == "pass-word-1");
  assert(fake::nvs["grok-wifi/token"] == token && fake::nvs["grok-wifi/host"] == "192.168.1.20");
  assert(fake::nvs["grok-wifi/port"] == "8767" && fake::nvs["grok-wifi/ca"] == Ca);
  assert(ESP.restarts == 1);

  // A gateway mDNS name is stored as given (hub ADR 006).
  command(provision("home-net", "pass-word-1", token, "my-mac.local"));
  assert(reply["ok"] && fake::nvs["grok-wifi/host"] == "my-mac.local");
  assert(grok::validLocalName("a-1.b.local") && !grok::validLocalName("My-Mac.local"));
  assert(!grok::validLocalName("mac-.local") && !grok::validLocalName("mac.local.evil"));

  // An open network has an empty password.
  command(provision("open-net", "", token));
  assert(reply["ok"] && fake::nvs["grok-wifi/pass"].empty());

  // Factory reset erases the namespace and restarts.
  command(R"({"type":"factory_reset"})");
  assert(reply["ok"] && reply["restarting"]);
  assert(fake::nvs.empty() && ESP.restarts == 4);

  // A failing NVS write is reported.
  fake::nvsFails = true;
  command(provision("home-net", "pass-word-1", token));
  assert(reply["error"]["code"] == "storage_failed");
  std::cout << "Wi-Fi console tests passed\n";
}
