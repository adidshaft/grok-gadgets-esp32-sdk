// SPDX-License-Identifier: Apache-2.0
// Generic ESP32-S3 board: one plain LED (`led.set`), one button and a custom `button.held`
// event. Wire an LED with a resistor to LedPin. ButtonPin 0 is the BOOT button on most
// ESP32-S3 dev boards. Do not use RGB_BUILTIN/LED_BUILTIN: the generic variant says GPIO48,
// which is wrong for many boards.
#include <GrokSession.h>

constexpr int LedPin = 4, ButtonPin = 0;
constexpr uint32_t HoldMs = 1000;
bool ledOn = false;
uint32_t pressedAt = 0;
bool held = false;
grok::Gadget gadget("Generic ESP32-S3 LED and button", "0.1.0", "esp32s3");

void reportState(JsonObject state, void *) { state["led"]["on"] = ledOn; }
grok::Error setLed(JsonObjectConst args, void *) {
  if (args.size() != 1 || !args["on"].is<bool>())
    return {"invalid_arguments", "Expected {\"on\": boolean}"};
  ledOn = args["on"];
  digitalWrite(LedPin, ledOn ? HIGH : LOW);
  return {};
}
void setup() {
  pinMode(LedPin, OUTPUT);
  digitalWrite(LedPin, LOW);
  gadget.state(reportState);
  gadget.command("led.set", setLed, nullptr,
                 R"({"type":"object","properties":{"on":{"type":"boolean"}},)"
                 R"("required":["on"],"additionalProperties":false})");
  gadget.event("button.held", R"({"properties":{"ms":{"type":"integer"}}})");
  gadget.button(ButtonPin);
  gadget.begin();
}
void loop() {
  bool pressed = digitalRead(ButtonPin) == LOW;
  if (!pressed) {
    pressedAt = 0;
    held = false;
  } else if (!pressedAt) {
    pressedAt = millis() | 1;
  } else if (!held && millis() - pressedAt >= HoldMs) {
    held = true;
    StaticJsonDocument<64> data;
    data["ms"] = millis() - pressedAt;
    gadget.emit("button.held", data.as<JsonObjectConst>());
  }
  gadget.loop();
}
