// SPDX-License-Identifier: Apache-2.0
// A scripted relay for the builder example tests (hub ADR 013 §6): the example joins with saved
// Wi-Fi, and each test hands it commands, reads its frames and checks the result.
#pragma once
#include "host_board.h"
#include <cassert>
#include <deque>
#include <string>
#include <vector>

namespace relay {
std::deque<std::string> commands; // Command objects for the next polls, one per poll.
std::vector<std::string> frames;  // Everything the gadget sent, in order.

std::string &wireOut() {
  return static_cast<WiFiClientSecure &>(app.linkForTests().stream()).output;
}
std::string &wireIn() { return static_cast<WiFiClientSecure &>(app.linkForTests().stream()).input; }

std::string answer(const std::string &frame) {
  if (frame.find("\"type\":\"hello\"") != std::string::npos)
    return R"({"ok":true,"session_id":"example","protocol_version":"0.1.0"})";
  if (frame.find("\"type\":\"poll\"") == std::string::npos || commands.empty())
    return frame.find("\"type\":\"poll\"") != std::string::npos ? R"({"ok":true,"commands":[]})"
                                                                : R"({"ok":true})";
  std::string command = commands.front();
  commands.pop_front();
  return R"({"ok":true,"commands":[)" + command + "]}";
}

// Runs the sketch for `ms` milliseconds of host time, answering every frame.
void run(uint32_t ms) {
  uint32_t until = fake::now + ms;
  while (fake::now < until) {
    loop();
    fake::now += 1;
    size_t end;
    while ((end = wireOut().find('\n')) != std::string::npos) {
      std::string frame = wireOut().substr(0, end);
      wireOut().erase(0, end + 1);
      frames.push_back(frame);
      wireIn() += answer(frame) + "\n";
    }
  }
}

void start() {
  fake::nvs = {{"grok-wifi/ssid", "example-net"},
               {"grok-wifi/pass", "example-pass"},
               {"grok-wifi/token", "example-device-token"},
               {"grok-wifi/host", "devices.example.net"},
               {"grok-wifi/port", "8444"},
               {"grok-wifi/ca", "-----BEGIN CERTIFICATE-----\nMIIBexampleOnlyNotACertificate0123\n"
                                "-----END CERTIFICATE-----\n"}};
  WiFi.state = WL_CONNECTED;
  WiFi.dns["devices.example.net"] = IPAddress(203, 0, 113, 7);
  setup();
  run(1000);
  assert(app.online());
}

// The first frame after `from` that contains every fragment, or "" when none does.
std::string find(size_t from, std::initializer_list<const char *> fragments) {
  for (size_t i = from; i < frames.size(); ++i) {
    bool all = true;
    for (auto fragment : fragments)
      all = all && frames[i].find(fragment) != std::string::npos;
    if (all)
      return frames[i];
  }
  return "";
}
size_t count(size_t from, const char *fragment) {
  size_t total = 0;
  for (size_t i = from; i < frames.size(); ++i)
    total += frames[i].find(fragment) != std::string::npos;
  return total;
}

// Sends one command and returns its acknowledgement frame.
std::string command(const std::string &id, const std::string &capability,
                    const std::string &arguments) {
  size_t from = frames.size();
  commands.push_back(R"({"command_id":")" + id + R"(","capability":")" + capability +
                     R"(","arguments":)" + arguments + "}");
  std::string ack;
  for (int i = 0; i < 50 && ack.empty(); ++i) {
    run(100);
    ack = find(from, {"\"type\":\"ack\"", ("\"command_id\":\"" + id + "\"").c_str()});
  }
  assert(!ack.empty());
  return ack;
}
bool executed(const std::string &ack) { return ack.find("\"executed\"") != std::string::npos; }
bool refused(const std::string &ack) {
  return !executed(ack) && ack.find("invalid_arguments") != std::string::npos;
}
} // namespace relay
