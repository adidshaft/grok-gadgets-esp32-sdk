// SPDX-License-Identifier: Apache-2.0
// Servo (hub ADR 013 §6): an SG90 hobby servo on GPIO6, powered from 5 V with a common
// ground, points or opens something, such as a pet-feeder flap. `servo.move {angle}` moves it
// to 0-180 degrees at about 180 degrees a second; the acknowledgement comes after the move
// ends, so Grok Bot's gadgets_command_status says when it finished. Out-of-range angles are
// refused, and the servo starts at 90 degrees.
// Not tested on hardware until someone runs it with the part; host models cover the logic.
#include <GrokApp.h>

constexpr int ServoPin = 6, Channel = 0, Bits = 16;
constexpr uint32_t Hz = 50, MinPulseUs = 500, MaxPulseUs = 2500, StepMs = 5;

grok::Gadget gadget("Servo", GROK_APP_VERSION, "servo");
grok::WifiApp app(gadget);
int angle = 90;

void write(int degrees) {
  uint32_t pulse = MinPulseUs + uint32_t(degrees) * (MaxPulseUs - MinPulseUs) / 180;
  ledcWrite(Channel, pulse * ((1u << Bits) - 1) / (1000000 / Hz)); // Duty for a 20 ms period.
}
void reportState(JsonObject state, void *) { state["servo"]["angle"] = angle; }
grok::Error moveServo(JsonObjectConst args, void *) {
  if (args.size() != 1 || !args["angle"].is<int>())
    return {"invalid_arguments", "Expected {\"angle\": integer from 0 to 180}"};
  int target = args["angle"];
  if (target < 0 || target > 180)
    return {"invalid_arguments", "angle must be from 0 to 180"};
  while (angle != target) { // About 1 degree every 5 ms; the ACK waits for the end.
    angle += target > angle ? 1 : -1;
    write(angle);
    delay(StepMs);
  }
  return {};
}

void setup() {
  ledcSetup(Channel, Hz, Bits);
  ledcAttachPin(ServoPin, Channel);
  write(angle);
  gadget.state(reportState);
  gadget.command("servo.move", moveServo, nullptr,
                 R"({"type":"object","description":"Move the servo to an angle; the )"
                 R"(result comes when the move ends","properties":{"angle":{"type":"integer",)"
                 R"("minimum":0,"maximum":180}},"required":["angle"],)"
                 R"("additionalProperties":false})");
  gadget.button(0);
  app.begin();
}

void loop() { app.loop(); }
