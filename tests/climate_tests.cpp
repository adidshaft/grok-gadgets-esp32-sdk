// SPDX-License-Identifier: Apache-2.0
// The climate sensor example (hub ADR 013 §6) against a scripted relay: threshold.set, its
// refusals, threshold.crossed both ways, and a module that goes missing and comes back.
#define GROK_HOST_SIMULATION
#include "../examples/climate-sensor/main.cpp"
#include "example_relay.h"
#include <iostream>

int main() {
  relay::start();
  using relay::command;
  using relay::executed;
  using relay::refused;

  // An integer is a number too; the state reports the new threshold at once.
  std::string ack = command("t1", "threshold.set", R"({"above_c":24})");
  assert(executed(ack) && ack.find("\"above_c\":24") != std::string::npos);
  int bad = 0;
  for (auto arguments : {R"({"above_c":"warm"})", R"({"above_c":86})", R"({"above_c":-41})",
                         R"({})", R"({"above_c":24,"unit":"F"})"}) {
    ack = command("bad" + std::to_string(++bad), "threshold.set", arguments);
    assert(refused(ack) && aboveC == 24);
  }

  // The host model swings 22 -> 28 -> 22 degrees, a degree every 2 seconds: one crossing up
  // past 24 and one back down, each with the reading that crossed.
  size_t from = relay::frames.size();
  relay::run(26000);
  std::string up = relay::find(from, {"threshold.crossed", "\"above\":true"});
  std::string down = relay::find(from, {"threshold.crossed", "\"above\":false"});
  assert(up.find("\"reading\":\"temperature_c\"") != std::string::npos);
  assert(up.find("\"value\":25") != std::string::npos);
  assert(down.find("\"value\":24") != std::string::npos);
  assert(relay::count(from, "threshold.crossed") == 2);
  assert(relay::find(from, {"\"type\":\"state\"", "\"temperature_c\""}) != "");

  // Unplugged: after three failed reads the state says missing and drops the stale reading.
  hostSensorPresent = false;
  relay::run(8000);
  assert(!sensorOk && !haveReading);
  from = relay::frames.size();
  relay::run(3000);
  ack = command("t2", "threshold.set", R"({"above_c":30})");
  assert(executed(ack) && ack.find("\"sensor\":\"missing\"") != std::string::npos &&
         ack.find("temperature_c") == std::string::npos);
  // Plugged back in: found within 5 seconds, with no crossing for the first reading.
  hostSensorPresent = true;
  from = relay::frames.size();
  relay::run(9000);
  assert(sensorOk && haveReading);
  assert(relay::count(from, "threshold.crossed") == 0);
  std::cout << "climate tests passed\n";
}
