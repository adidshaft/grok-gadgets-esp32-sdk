// SPDX-License-Identifier: Apache-2.0
// Project template (hub ADR 013 §5): copy this folder to start a gadget. It has the relay
// link, phone setup, claim codes, signed updates (build with GROK_UPDATE_PUBLIC_KEY) and the
// BOOT factory reset through grok::WifiApp. Replace the three marked parts with your own.
#include <GrokApp.h>

// 1. Name the gadget and its device ID prefix (the ID becomes "mygadget-" and the chip's MAC).
grok::Gadget gadget("My gadget", GROK_APP_VERSION, "mygadget");
grok::WifiApp app(gadget);
bool on = false;

// 2. Report what the gadget is doing. Grok Bot reads this with gadgets_get_state.
void reportState(JsonObject state, void *) { state["switch"]["on"] = on; }

// 3. One handler for each command. Check the arguments; return an error to refuse.
grok::Error setSwitch(JsonObjectConst args, void *) {
  if (args.size() != 1 || !args["on"].is<bool>())
    return {"invalid_arguments", "Expected {\"on\": boolean}"};
  on = args["on"]; // Your hardware code goes here.
  return {};
}

void setup() {
  gadget.state(reportState);
  // The description is what Grok Bot reads: say what the command does, in one sentence.
  gadget.command("switch.set", setSwitch, nullptr,
                 R"({"type":"object","description":"Turn my gadget on or off",)"
                 R"("properties":{"on":{"type":"boolean"}},"required":["on"],)"
                 R"("additionalProperties":false})");
  gadget.button(0); // BOOT: presses reach Grok Bot as events; held 10 s it resets the gadget.
  app.begin();
}

void loop() { app.loop(); }
