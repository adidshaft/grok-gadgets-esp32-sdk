// SPDX-License-Identifier: Apache-2.0
// Switch (hub ADR 013 §6): a 3.3 V opto-isolated relay module on GPIO5 switches a
// LOW-VOLTAGE load, such as a 5 V or 12 V DC lamp or fan. NEVER wire mains voltage to it:
// mains wiring needs an approved product and a qualified person.
//
// `switch.set {on}` turns the load on or off; the state reports `switch.on`. The load is off
// at boot, and it turns off by itself when the relay link has been down for 30 seconds, so a
// lost connection never leaves it on. Most relay modules switch on when the input is LOW;
// build with -DSWITCH_ACTIVE_HIGH for a module that switches on when it is HIGH.
// Not tested on hardware until someone runs it with the part; host models cover the logic.
#include <GrokApp.h>

constexpr int RelayPin = 5;
constexpr uint32_t OfflineOffMs = 30000;
#ifdef SWITCH_ACTIVE_HIGH
constexpr int OnLevel = HIGH, OffLevel = LOW;
#else
constexpr int OnLevel = LOW, OffLevel = HIGH;
#endif

grok::Gadget gadget("Low-voltage switch", GROK_APP_VERSION, "switch");
grok::WifiApp app(gadget);
bool on = false;
bool offByLinkLoss = false;

void drive(bool value) {
  on = value;
  digitalWrite(RelayPin, on ? OnLevel : OffLevel);
}
void reportState(JsonObject state, void *) {
  auto sw = state.createNestedObject("switch");
  sw["on"] = on;
  if (offByLinkLoss)
    sw["off_reason"] = "link_lost";
}
grok::Error setSwitch(JsonObjectConst args, void *) {
  if (args.size() != 1 || !args["on"].is<bool>())
    return {"invalid_arguments", "Expected {\"on\": boolean}"};
  drive(args["on"].as<bool>()); // Setting the same value again is safe: retries never toggle.
  offByLinkLoss = false;
  return {};
}

void setup() {
  digitalWrite(RelayPin, OffLevel); // Off before the pin becomes an output.
  pinMode(RelayPin, OUTPUT);
  drive(false);
  gadget.state(reportState);
  gadget.command("switch.set", setSwitch, nullptr,
                 R"({"type":"object","description":"Turn the low-voltage load on or off. )"
                 R"(It turns off by itself after 30 s without the relay",)"
                 R"("properties":{"on":{"type":"boolean"}},"required":["on"],)"
                 R"("additionalProperties":false})");
  gadget.button(0);
  app.begin();
}

void loop() {
  app.loop();
  if (on && app.offlineFor() >= OfflineOffMs) {
    drive(false); // Safe-off on link loss.
    offByLinkLoss = true;
  }
}
