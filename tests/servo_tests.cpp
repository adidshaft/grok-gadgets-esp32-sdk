// SPDX-License-Identifier: Apache-2.0
// The servo example (hub ADR 013 §6) against a scripted relay: the acknowledgement comes after
// the move ends, the pulse matches the angle, and out-of-range angles change nothing.
#define GROK_HOST_SIMULATION
#include "../examples/servo/main.cpp"
#include "example_relay.h"
#include <iostream>

uint32_t pulseUs() { return uint32_t(uint64_t(fake::ledcDuty()) * 20000 / 65535 + 1); }

int main() {
  relay::start();
  using relay::command;
  assert(angle == 90 && pulseUs() / 10 == 150); // Starts centred: 1500 us.

  // 90 degrees at 5 ms a degree: the acknowledgement waits at least 450 ms.
  size_t from = relay::frames.size();
  uint32_t sent = fake::now;
  std::string ack = command("m1", "servo.move", R"({"angle":0})");
  assert(relay::executed(ack) && ack.find("\"angle\":0") != std::string::npos);
  assert(fake::now - sent >= 450);
  assert(angle == 0 && pulseUs() / 10 == 50); // 500 us.
  ack = command("m2", "servo.move", R"({"angle":180})");
  assert(relay::executed(ack) && angle == 180 && pulseUs() / 10 == 250); // 2500 us.

  int bad = 0;
  for (auto arguments : {R"({"angle":181})", R"({"angle":-1})", R"({"angle":"left"})",
                         R"({"angle":45.5})", R"({})", R"({"angle":45,"speed":2})"}) {
    ack = command("bad" + std::to_string(++bad), "servo.move", arguments);
    assert(relay::refused(ack) && angle == 180);
  }
  // A repeated command ID is answered from the cache: the servo does not move again.
  ack = command("m1", "servo.move", R"({"angle":0})");
  assert(relay::executed(ack) && angle == 180);
  assert(relay::count(from, "\"type\":\"ack\"") == 9);
  std::cout << "servo tests passed\n";
}
