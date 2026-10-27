// SPDX-License-Identifier: Apache-2.0
// The switch example (hub ADR 013 §6) against a scripted relay: off at boot, on and off,
// repeated values never toggle, refusals, and the safe-off after 30 s without the relay.
// app_tests covers phone setup and the claim code with the same sketch.
#define GROK_HOST_SIMULATION
#include "../examples/switch/main.cpp"
#include "example_relay.h"
#include <iostream>

int main() {
  relay::start();
  using relay::command;
  using relay::executed;
  assert(!on && fake::led == HIGH); // Off at boot; the module switches on when its input is LOW.

  std::string ack = command("s1", "switch.set", R"({"on":true})");
  assert(executed(ack) && ack.find("\"on\":true") != std::string::npos);
  assert(on && fake::led == LOW);
  ack = command("s2", "switch.set", R"({"on":true})"); // The same value again stays on.
  assert(executed(ack) && on);
  int bad = 0;
  for (auto arguments : {R"({"on":"yes"})", R"({"on":1})", R"({})", R"({"on":false,"for_s":5})"}) {
    ack = command("bad" + std::to_string(++bad), "switch.set", arguments);
    assert(relay::refused(ack) && on);
  }
  ack = command("s3", "switch.set", R"({"on":false})");
  assert(executed(ack) && !on && fake::led == HIGH);

  // On, then the relay link drops: still on at 29 s, off with a reason by 31 s.
  command("s4", "switch.set", R"({"on":true})");
  auto &tls = static_cast<WiFiClientSecure &>(app.linkForTests().stream());
  tls.accept = false;
  tls.open = false;
  uint32_t dropped = fake::now;
  relay::run(29000);
  assert(on && fake::now - dropped < 30000);
  relay::run(2000);
  assert(!on && fake::led == HIGH && offByLinkLoss);
  std::cout << "switch tests passed\n";
}
