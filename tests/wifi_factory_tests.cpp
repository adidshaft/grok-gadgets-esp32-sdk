// SPDX-License-Identifier: Apache-2.0
// ESP-SETUP-002 (hub ADR 010): the factory identity is separate from the user's Wi-Fi settings.
#define OLED_KIT_WIFI
#include "../examples/oled-kit/main.cpp"
#include "host_board.h"
#include <cassert>
#include <iostream>

const char *Ca =
    "-----BEGIN CERTIFICATE-----\nMIIBtestCertificateBodyOnlyForHostTests0123456789abcdef\n"
    "-----END CERTIFICATE-----\n";
const char *Token = "device-token-0123456789";
const char *Passphrase = "K7QXM2PAHJ4R9TVW";
const char *Code = "ABCD2345";
DynamicJsonDocument reply(1024);

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
std::string factoryLine(const char *passphrase = Passphrase, const char *code = Code,
                        const char *host = "devices.example.net", int replace = -1) {
  DynamicJsonDocument doc(2048);
  doc["type"] = "factory_provision";
  doc["token"] = Token;
  doc["host"] = host;
  doc["port"] = 8444;
  doc["ca"] = Ca;
  doc["passphrase"] = passphrase;
  doc["setup_code"] = code;
  if (replace >= 0)
    doc["replace"] = replace == 1;
  std::string line;
  serializeJson(doc, line);
  return line;
}
void secretFree(const std::string &text) {
  for (const char *secret : {Token, Passphrase, Code, "home-net", "pass-word-1"})
    assert(text.find(secret) == std::string::npos);
}
std::string json() {
  std::string text;
  serializeJson(reply, text);
  return text;
}

int main() {
  setup();
  command(R"({"type":"status"})");
  assert(reply["factory"] == false && reply["provisioned"] == false);

  // Bad label values are refused without echoing anything; nothing is stored.
  for (auto line :
       {factoryLine("short"), factoryLine("K7QXM2PAHJ4R9TV0"), factoryLine("k7qxm2pahj4r9tvw"),
        factoryLine(Passphrase, "ABCD234"), factoryLine(Passphrase, "ABCD234O"),
        factoryLine(Passphrase, Code, "gateway")}) {
    command(line);
    assert(reply["ok"] == false && reply["error"]["code"] == "invalid_settings");
    secretFree(json());
  }
  command(R"({"type":"factory_provision","token":"device-token-0123456789"})");
  assert(reply["error"]["code"] == "invalid_settings");
  assert(fake::nvs.empty() && ESP.restarts == 0);
  assert(grok::validSetupCode("ABCD2345") && !grok::validSetupCode("ABCD-2345"));

  // A valid identity goes to grok-factory only; the reply carries no value.
  command(factoryLine());
  assert(reply["ok"] && reply["restarting"]);
  secretFree(json());
  assert(fake::nvs["grok-factory/token"] == Token);
  assert(fake::nvs["grok-factory/host"] == "devices.example.net");
  assert(fake::nvs["grok-factory/port"] == "8444" && fake::nvs["grok-factory/ca"] == Ca);
  assert(fake::nvs["grok-factory/passphrase"] == Passphrase);
  assert(fake::nvs["grok-factory/setup_code"] == Code);
  for (auto &entry : fake::nvs)
    assert(entry.first.rfind("grok-factory/", 0) == 0);
  assert(ESP.restarts == 1);

  // A second identity needs replace.
  command(factoryLine(Passphrase, "WXYZ6789"));
  assert(reply["error"]["code"] == "already_provisioned");
  secretFree(json());
  assert(fake::nvs["grok-factory/setup_code"] == Code);
  command(factoryLine(Passphrase, "WXYZ6789", "devices.example.net", 1));
  assert(reply["ok"] && fake::nvs["grok-factory/setup_code"] == "WXYZ6789");
  command(factoryLine());
  assert(reply["error"]["code"] == "already_provisioned");
  command(factoryLine(Passphrase, Code, "devices.example.net", 1));
  assert(reply["ok"]);

  // Factory identity alone: not yet joined to Wi-Fi, but ready for the setup hotspot.
  grok::WifiSettings loaded;
  grok::FactorySettings stored;
  assert(!store.load(loaded) && !loaded.ssid[0]);
  assert(!strcmp(loaded.host, "devices.example.net") && loaded.port == 8444);
  assert(store.loadFactory(stored) && stored.setupReady());

  // The setup page stores only the home Wi-Fi; the link then has everything.
  assert(store.saveWifi("home-net", "pass-word-1"));
  assert(fake::nvs.count("grok-wifi/token") == 0);
  assert(store.load(loaded) && !strcmp(loaded.ssid, "home-net"));
  assert(!strcmp(loaded.token, Token) && !strcmp(loaded.ca, Ca));

  // A BOOT reset (or factory_reset) erases the user's settings only.
  command(R"({"type":"factory_reset"})");
  assert(reply["ok"]);
  assert(fake::nvs.count("grok-wifi/ssid") == 0 && fake::nvs.count("grok-wifi/pass") == 0);
  assert(fake::nvs["grok-factory/token"] == Token && fake::nvs["grok-factory/setup_code"] == Code);
  assert(!store.load(loaded) && store.loadFactory(stored) && stored.setupReady());

  // A full USB provision line (pre-ADR 010) still works and takes precedence over the factory.
  fake::nvs["grok-wifi/ssid"] = "home-net";
  fake::nvs["grok-wifi/pass"] = "pass-word-1";
  fake::nvs["grok-wifi/token"] = "usb-token-0123456789";
  fake::nvs["grok-wifi/host"] = "192.168.1.20";
  fake::nvs["grok-wifi/port"] = "8767";
  fake::nvs["grok-wifi/ca"] = Ca;
  assert(store.load(loaded) && !strcmp(loaded.token, "usb-token-0123456789"));
  assert(!strcmp(loaded.host, "192.168.1.20") && loaded.port == 8767);

  // A board provisioned before ADR 010 has no factory namespace and keeps working.
  for (auto it = fake::nvs.begin(); it != fake::nvs.end();)
    it = it->first.rfind("grok-factory/", 0) == 0 ? fake::nvs.erase(it) : std::next(it);
  assert(store.load(loaded) && !store.loadFactory(stored) && !stored.setupReady());

  // A failing NVS write is reported.
  fake::nvsFails = true;
  command(factoryLine());
  assert(reply["error"]["code"] == "storage_failed");
  std::cout << "Wi-Fi factory tests passed\n";
}
