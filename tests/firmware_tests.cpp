// SPDX-License-Identifier: Apache-2.0
#include "../src/main.cpp"
#include "host_board.h"
#include <cassert>
#include <iostream>
DynamicJsonDocument frame(4096);
void take(const char *type) {
  assert(!Serial.output.empty());
  assert(Serial.output.back() == '\n');
  assert(!deserializeJson(frame, Serial.output));
  Serial.output.clear();
  assert(frame["type"] == type);
}
void reply(const char *json) {
  Serial.input = std::string(json) + "\n";
  loop();
}
void advance(uint32_t ms) {
  fake::now += ms;
  loop();
}
void nextCommand(const char *response) {
  reply(R"({"ok":true})");
  advance(100);
  take("poll");
  reply(response);
  take("ack");
}
std::string ackJson() {
  std::string json;
  serializeJson(frame, json);
  return json;
}
int main() {
  setup();
  assert(fake::pixel == 0 && Serial.rxBuffer >= 2 * grok::MaxFrame);
  advance(500);
  take("hello");
  assert(frame["protocol_version"] == "0.1.0");
  assert(frame["device"]["model"] == grok::c124::Model);
  // Device ID prints MAC bytes in transmission order (FakeEsp MAC bc:9a:78:56:34:12).
  assert(frame["device"]["device_id"] == "c124-bc9a78563412");
  std::string caps;
  serializeJson(frame["device"]["capabilities"], caps);
  assert(caps == R"(["rgb.set","button","state","history_lost"])");
  assert(!frame["device"].containsKey("capability_schemas"));
  reply(R"({"ok":true,"session_id":"session-1","protocol_version":"0.1.0"})");
  assert(gadget.connected() && !gadget.waiting());
  advance(100);
  take("poll");
  reply(
      R"({"ok":true,"commands":[{"command_id":"cmd-1","capability":"rgb.set","arguments":{"r":0,"g":255,"b":0,"on":true}}]})");
  take("ack");
  assert(frame["status"] == "executed" && fake::pixel == 0xff00);
  assert(frame["state"]["button"]["pressed"] == false);
  int shows = fake::shows;
  auto original = ackJson();
  const char *green =
      R"({"ok":true,"commands":[{"command_id":"cmd-1","capability":"rgb.set","arguments":{"r":0,"g":255,"b":0,"on":true}}]})";
  for (int retry = 0; retry < 4; ++retry) {
    nextCommand(green);
    assert(ackJson() == original && fake::shows == shows);
  }
  nextCommand(
      R"({"ok":true,"commands":[{"command_id":"cmd-1","capability":"rgb.set","arguments":{"r":255,"g":0,"b":0,"on":true}}]})");
  assert(frame["error"]["code"] == "duplicate_conflict" && fake::shows == shows);
  const char *bad =
      R"({"ok":true,"commands":[{"command_id":"retry-bad","capability":"rgb.set","arguments":{"r":256,"g":0,"b":0,"on":true}}]})";
  nextCommand(bad);
  assert(frame["status"] == "failed" && frame["error"]["code"] == "invalid_arguments");
  auto failed = ackJson();
  for (int retry = 0; retry < 4; ++retry) {
    nextCommand(green);
    assert(ackJson() == original && fake::shows == shows);
    nextCommand(bad);
    assert(ackJson() == failed && fake::shows == shows);
  }
  reply(R"({"ok":true})");
  fake::pin = 0;
  loop();
  advance(30);
  take("event");
  assert(frame["name"] == "button" && frame["data"]["pressed"] == true);
  reply(R"({"ok":true})");
  fake::pin = 1;
  loop();
  advance(30);
  take("event");
  assert(frame["data"]["pressed"] == false);
  reply(R"({"ok":true})");
  advance(100);
  take("poll");
  reply(
      R"({"ok":true,"commands":[{"command_id":"bad-1","capability":"rgb.set","arguments":{"r":256,"g":0,"b":0,"on":true}}]})");
  take("ack");
  assert(frame["status"] == "failed" && fake::shows == shows);
  // Lost acknowledgement starts a fresh session; no action is replayed.
  advance(grok::RequestTimeoutMs + 1);
  assert(!gadget.connected() && !gadget.pendingAck() && !gadget.waiting() && fake::shows == shows);
  advance(500);
  take("hello");
  reply(R"({"ok":true,"session_id":"session-2","protocol_version":"0.1.0"})");
  take("state"); // The state interval elapsed during the timeout.
  reply(R"({"ok":true})");
  advance(100);
  take("poll");
  // Hold one poll response while 20 debounced edges fill the bounded queue.
  for (int i = 0; i < 20; ++i) {
    fake::pin = i % 2;
    loop();
    advance(30);
  }
  assert(gadget.queued() == 16 && gadget.lost() == 4);
  reply(R"({"ok":true,"commands":[]})");
  for (int i = 0; i < 16; ++i) {
    take("event");
    assert(frame["name"] == "button");
    reply(R"({"ok":true})");
  }
  take("event");
  assert(frame["name"] == "history_lost" && frame["data"]["dropped"] == 4);
  reply(R"({"ok":true})");
  assert(gadget.lost() == 0 && gadget.queued() == 0 && gadget.connected());
  advance(100);
  take("poll");
  reply(R"({"ok":false,"error":{"code":"revoked","message":"Credential revoked"}})");
  assert(!gadget.connected());
  std::cout << "firmware host simulation: hello/poll/ACK, repeated success/failure retries and "
               "interleaved/conflicting commands, invalid RGB, real "
               "consumer button edges/overflow recovery, lost ACK and revocation recovery passed\n";
}
