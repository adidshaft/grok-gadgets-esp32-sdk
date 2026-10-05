// SPDX-License-Identifier: Apache-2.0
// Prints every frame an example sketch writes while a scripted gateway answers it.
// Arguments are command objects delivered one per poll. tools/check_contract.py validates
// the output against the pinned schema.
#define GROK_HOST_SIMULATION
#include "host_board.h"
#include <iostream>
#include <string>
#include <vector>
#include GROK_EXAMPLE

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
    while ((end = Serial.output.find('\n')) != std::string::npos) {
      std::string line = Serial.output.substr(0, end);
      Serial.output.erase(0, end + 1);
      std::cout << line << '\n';
      StaticJsonDocument<256> filter;
      filter["type"] = true;
      StaticJsonDocument<256> frame;
      deserializeJson(frame, line, DeserializationOption::Filter(filter));
      std::string answer = respond(frame["type"] | "");
      if (holding)
        held = answer;
      else
        Serial.input += answer + "\n";
    }
  }
}
} // namespace script
int main(int argc, char **argv) {
  using script::holding;
  using script::step;
  script::commands.assign(argv + 1, argv + argc);
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
  Serial.input += script::held + "\n";
  step(3000);
  step(4000);
}
