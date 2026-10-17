// SPDX-License-Identifier: Apache-2.0
// ESP-CLAIM-001 (hub ADR 011): claim codes in hello and poll replies, opt-in only.
#include "host_board.h"
#include <GrokSession.h>
#include <cassert>
#include <cstring>
#include <iostream>
#include <string>
grok::Gadget gadget("Claim test gadget", "0.0.1", "test");
DynamicJsonDocument frame(8192);
void take(const char *type) {
  assert(!Serial.output.empty() && Serial.output.back() == '\n');
  assert(!deserializeJson(frame, Serial.output));
  Serial.output.clear();
  assert(frame["type"] == type);
}
void reply(const std::string &json) {
  Serial.input = json + "\n";
  gadget.loop();
}
void advance(uint32_t ms) {
  fake::now += ms;
  gadget.loop();
}
// Run until the gadget sends its next request; return its type.
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
const std::string base = R"({"ok":true,"session_id":"s","protocol_version":"0.1.0")";
std::string helloReply(const std::string &extra = "") { return base + extra + "}"; }
bool code(const char *expected) {
  const char *shown = gadget.claimCode();
  return expected ? shown && !strcmp(shown, expected) : shown == nullptr;
}

// A gadget that opted in sends the claim field and accepts the two reply fields.
void opted_in_hello_and_poll() {
  assert(next() == "hello");
  assert(frame["claim"] == "ready");
  reply(helloReply(R"(,"claimed":false,"claim_code":"ABCD-EFGH")"));
  assert(gadget.connected() && !gadget.claimed() && code("ABCD-EFGH"));
  // An ordinary poll reply keeps the code.
  assert(awaitPoll() == "poll");
  reply(R"({"ok":true,"commands":[]})");
  assert(gadget.connected() && code("ABCD-EFGH"));
  // A fresh code arrives in a poll reply.
  assert(awaitPoll() == "poll");
  reply(R"({"ok":true,"commands":[],"claimed":false,"claim_code":"JKLM-NPQR"})");
  assert(gadget.connected() && code("JKLM-NPQR"));
  // Claimed: the code goes.
  assert(awaitPoll() == "poll");
  reply(R"({"ok":true,"commands":[],"claimed":true})");
  assert(gadget.connected() && gadget.claimed() && code(nullptr));
}

// Unknown fields and malformed claim fields still reset the session.
void strict_replies() {
  assert(awaitPoll() == "poll");
  reply(R"({"ok":true,"commands":[],"other":1})");
  assert(!gadget.connected());
  assert(next() == "hello");
  reply(helloReply(R"(,"claimed":false,"claim_code":"ABCDEFGH")")); // no dash
  assert(!gadget.connected());
  assert(next() == "hello");
  reply(helloReply(R"(,"claimed":"no")"));
  assert(!gadget.connected());
  // A relay in operator mode answers without the fields: no code, still Online.
  assert(next() == "hello");
  reply(helloReply());
  assert(gadget.connected() && code(nullptr));
}

// A disconnect clears the code: it is only valid while the session lives.
void disconnect_clears_the_code() {
  assert(awaitPoll() == "poll");
  reply(R"({"ok":true,"commands":[],"claimed":false,"claim_code":"STUV-WXYZ"})");
  assert(code("STUV-WXYZ"));
  assert(awaitPoll() == "poll");
  reply(R"({"ok":false,"error":{"code":"stale_session","message":"m"}})");
  assert(!gadget.connected() && code(nullptr));
}

// The first hello after a phone setup says after_setup; later hellos say ready again.
void after_setup_once() {
  gadget.claimAfterSetup();
  assert(next() == "hello");
  assert(frame["claim"] == "after_setup");
  reply(helloReply(R"(,"claimed":true,"claim_code":"ABCD-EFGH")"));
  assert(gadget.connected() && gadget.claimed() && code("ABCD-EFGH"));
  assert(awaitPoll() == "poll");
  reply(R"({"ok":false,"error":{"code":"stale_session","message":"m"}})");
  assert(next() == "hello");
  assert(frame["claim"] == "ready");
  reply(helloReply(R"(,"claimed":true)"));
  assert(gadget.connected() && code(nullptr));
}

// A relay from before claim codes refuses the field: ask once more without it, then never
// again until the next boot.
void old_relay_falls_back() {
  assert(awaitPoll() == "poll");
  reply(R"({"ok":false,"error":{"code":"stale_session","message":"m"}})");
  assert(next() == "hello" && frame["claim"] == "ready");
  reply(R"({"ok":false,"error":{"code":"invalid_request","message":"m"}})");
  assert(!gadget.connected());
  assert(next() == "hello");
  assert(frame["claim"].isNull());
  reply(helloReply());
  assert(gadget.connected() && code(nullptr));
  // Without the field the strict reply shape of 0.1.0 holds again.
  assert(awaitPoll() == "poll");
  reply(R"({"ok":true,"commands":[],"claimed":true})");
  assert(!gadget.connected());
}

int main() {
  gadget.showClaimCodes();
  assert(gadget.begin());
  opted_in_hello_and_poll();
  strict_replies();
  disconnect_clears_the_code();
  after_setup_once();
  old_relay_falls_back();
  std::cout << "claim tests passed\n";
}
