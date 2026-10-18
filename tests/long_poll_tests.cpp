// SPDX-License-Identifier: Apache-2.0
// GW-PROTO-002 (hub ADR 014): long-poll, opt-in only. A poll carries wait_ms and the relay
// answers when a command is ready or the wait ends.
#include "host_board.h"
#include <GrokSession.h>
#include <cassert>
#include <cstring>
#include <iostream>
#include <string>
grok::Gadget gadget("Long-poll test gadget", "0.0.1", "test");
DynamicJsonDocument frame(8192);
void reply(const std::string &json) {
  Serial.input = json + "\n";
  gadget.loop();
}
void advance(uint32_t ms) {
  fake::now += ms;
  gadget.loop();
}
std::string next() {
  for (uint32_t i = 0; i < grok::MaxBackoffMs + 1000 && Serial.output.empty(); ++i)
    advance(1);
  assert(!Serial.output.empty() && !deserializeJson(frame, Serial.output));
  Serial.output.clear();
  return frame["type"].as<std::string>();
}
std::string awaitPoll() {
  std::string type;
  while ((type = next()) == "state")
    reply(R"({"ok":true})");
  return type;
}
const std::string hello = R"({"ok":true,"session_id":"s","protocol_version":"0.1.0"})";

void limits() {
  grok::Gadget other("Limits", "0.0.1", "test");
  assert(!other.longPoll(99));
  assert(!other.longPoll(grok::StateIntervalMs)); // State must still fit between polls.
  assert(other.longPoll(100) && other.longPoll(2500));
}

void polls_wait_and_commands_still_arrive() {
  assert(next() == "hello");
  reply(hello);
  assert(gadget.connected());
  assert(awaitPoll() == "poll" && frame["wait_ms"] == 2500);
  reply(
      R"({"ok":true,"commands":[{"command_id":"c1","capability":"rgb.set","arguments":{"r":1,"g":2,"b":3,"on":true}}]})");
  assert(next() == "ack" && frame["command_id"] == "c1"); // No handler here: a failed ACK.
  reply(R"({"ok":true})");
  assert(awaitPoll() == "poll" && frame["wait_ms"] == 2500);
  reply(R"({"ok":true,"commands":[]})");
}

void state_still_goes_out_between_waiting_polls() {
  // The relay held this poll for the full wait: state is due and goes next.
  assert(awaitPoll() == "poll");
  fake::now += 2500;
  reply(R"({"ok":true,"commands":[]})");
  bool sawState = false;
  for (int i = 0; i < 3 && !sawState; ++i) {
    std::string type = next();
    sawState = type == "state";
    reply(type == "state" ? R"({"ok":true})" : R"({"ok":true,"commands":[]})");
  }
  assert(sawState);
}

void old_relay_keeps_the_session_and_drops_the_field() {
  assert(awaitPoll() == "poll" && frame["wait_ms"] == 2500);
  reply(R"({"ok":false,"error":{"code":"invalid_request","message":"m"}})");
  assert(gadget.connected()); // The relay keeps the session for invalid_request.
  assert(awaitPoll() == "poll" && frame["wait_ms"].isNull());
  reply(R"({"ok":true,"commands":[]})");
  assert(awaitPoll() == "poll" && frame["wait_ms"].isNull()); // Until the next boot.
  reply(R"({"ok":true,"commands":[]})");
}

void opt_in_only() {
  grok::Gadget plain("Plain", "0.0.1", "test");
  // A gadget that never calls longPoll() sends bare polls; session_tests covers its traffic.
  assert(plain.longPoll(2500)); // Only before begin() does it take effect.
}

int main() {
  limits();
  opt_in_only();
  assert(gadget.longPoll(2500));
  assert(gadget.begin());
  assert(!gadget.longPoll(1000)); // Too late after begin().
  polls_wait_and_commands_still_arrive();
  state_still_goes_out_between_waiting_polls();
  old_relay_keeps_the_session_and_drops_the_field();
  std::cout << "long-poll tests passed\n";
}
