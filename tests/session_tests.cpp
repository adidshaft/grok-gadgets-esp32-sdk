// SPDX-License-Identifier: Apache-2.0
// grok::Gadget session behaviour with a simulated serial port and clock.
#include "host_board.h"
#include <GrokSession.h>
#include <cassert>
#include <iostream>
int executions = 0;
grok::Error any(JsonObjectConst, void *) {
  ++executions;
  return {};
}
grok::Gadget gadget("Session test gadget", "0.0.1", "test");
DynamicJsonDocument frame(8192);
void take(const char *type) {
  assert(!Serial.output.empty() && Serial.output.back() == '\n');
  assert(!deserializeJson(frame, Serial.output));
  Serial.output.clear();
  assert(frame["type"] == type);
}
void none() { assert(Serial.output.empty()); }
void reply(const std::string &json) {
  Serial.input = json + "\n";
  gadget.loop();
}
void advance(uint32_t ms) {
  fake::now += ms;
  gadget.loop();
}
const char *ok = R"({"ok":true})";
const char *hello = R"({"ok":true,"session_id":"s","protocol_version":"0.1.0"})";
std::string error(const char *code) {
  return std::string(R"({"ok":false,"error":{"code":")") + code + R"(","message":"m"}})";
}
// Wait for the next hello (up to the maximum backoff) and accept it.
uint32_t connect() {
  uint32_t waited = 0;
  while (Serial.output.empty()) {
    advance(1);
    ++waited;
    assert(waited <= grok::MaxBackoffMs + 10);
  }
  take("hello");
  reply(hello);
  assert(gadget.connected());
  return waited;
}
// Run until the gadget sends its next request; return its type.
std::string next() {
  for (int i = 0; i < 1000 && Serial.output.empty(); ++i)
    advance(1);
  assert(!Serial.output.empty() && !deserializeJson(frame, Serial.output));
  Serial.output.clear();
  return frame["type"].as<std::string>();
}
// Answer periodic state frames until the next poll arrives.
void awaitPoll() {
  std::string type;
  while ((type = next()) == "state")
    reply(ok);
  assert(type == "poll");
}
void idlePoll() {
  awaitPoll();
  reply(R"({"ok":true,"commands":[]})");
}
std::string pollWith(const std::string &command) {
  return R"({"ok":true,"commands":[)" + command + "]}";
}

void registration() {
  static grok::Gadget full("m", "1", "t");
  char names[16][8];
  for (int i = 0; i < 14; ++i) {
    snprintf(names[i], sizeof(names[i]), "c%d", i);
    assert(full.command(names[i], any));
  }
  // 14 commands + state + history_lost already fill the 16-name hello.
  assert(!full.command("c14", any) && !full.event("e") && !full.button(1));
  static grok::Gadget reserved("m", "1", "t");
  for (const char *name : {"button", "state", "history_lost"}) {
    assert(!reserved.command(name, any));
    assert(!reserved.event(name));
  }
  assert(reserved.command("rgb.set", any));
  assert(!reserved.command("rgb.set", any) && !reserved.event("rgb.set"));
  assert(!reserved.command("x.set", any, nullptr, "[1]"));
  assert(!reserved.command("x.set", any, nullptr, "{"));
  assert(!reserved.command("y.set", any, nullptr, R"({"type":"object"} x)"));
  assert(reserved.event("tick") && !reserved.command("tick", any));
  static grok::Gadget rgbSchema("m", "1", "t");
  assert(!rgbSchema.command("rgb.set", any, nullptr, R"({"type":"object"})"));

  assert(full.begin());
  assert(!full.command("late", any));
  fake::now += 500;
  full.loop();
  take("hello");
  auto caps = frame["device"]["capabilities"].as<JsonArrayConst>();
  assert(caps.size() == 16);
  for (size_t i = 0; i < caps.size(); ++i)
    for (size_t j = i + 1; j < caps.size(); ++j)
      assert(strcmp(caps[i], caps[j]));
}

void helloContents() {
  take("hello");
  auto device = frame["device"];
  std::string caps;
  serializeJson(device["capabilities"], caps);
  assert(caps == R"(["any.set","tick","motion","button","state","history_lost"])");
  std::string schemas;
  serializeJson(device["capability_schemas"], schemas);
  assert(schemas == R"({"any.set":{"type":"object"},)"
                    R"("tick":{"type":"object","x-grok-gadgets-kind":"event"},)"
                    R"("motion":{"properties":{"level":{"type":"integer"}},)"
                    R"("x-grok-gadgets-kind":"event"}})");
  assert(device["device_id"] == "test-bc9a78563412");
  assert(device["state"]["button"]["pressed"] == false);
  reply(hello);
  assert(gadget.connected());
}

void emitRules() {
  StaticJsonDocument<256> data;
  assert(!gadget.emit("undeclared") && !gadget.emit("history_lost") && !gadget.emit("button"));
  data["text"] = std::string(200, 'x');
  assert(!gadget.emit("motion", data.as<JsonObjectConst>()));
  assert(gadget.queued() == 0 && gadget.lost() == 0);
  data.clear();
  data["level"] = 3;
  assert(gadget.emit("motion", data.as<JsonObjectConst>()));
  advance(1);
  take("event");
  assert(frame["name"] == "motion" && frame["data"]["level"] == 3);
  reply(ok);
  assert(gadget.emit("tick"));
  advance(1);
  take("event");
  std::string tickData;
  serializeJson(frame["data"], tickData);
  assert(frame["name"] == "tick" && tickData == "{}");
  reply(ok);
}

// ESP-01: a retried loss report keeps its event ID and count; later losses report next.
void lossRetry() {
  for (int i = 0; i < 17; ++i)
    gadget.emit("tick");
  assert(gadget.queued() == 16 && gadget.lost() == 1);
  for (int i = 0; i < 16; ++i) {
    advance(1);
    take("event");
    reply(ok);
  }
  advance(1);
  take("event");
  assert(frame["name"] == "history_lost" && frame["data"]["dropped"] == 1);
  std::string lossId = frame["event_id"];
  reply(error("gateway_unavailable"));
  assert(!gadget.connected());
  for (int i = 0; i < 18; ++i)
    gadget.emit("tick"); // 16 queue, two more lost while the first report is unconfirmed.
  connect();
  for (int i = 0; i < 16; ++i) {
    advance(1);
    take("event");
    assert(frame["name"] == "tick");
    reply(ok);
  }
  advance(1);
  take("event");
  assert(frame["name"] == "history_lost" && frame["event_id"] == lossId &&
         frame["data"]["dropped"] == 1);
  reply(ok);
  advance(1);
  take("event");
  assert(frame["name"] == "history_lost" && frame["event_id"] != lossId &&
         frame["data"]["dropped"] == 2);
  reply(ok);
  assert(gadget.lost() == 0 && gadget.connected());
}

// ESP-02: a permanently rejected event is dropped, counted and polling continues.
void rejectedEvents() {
  gadget.emit("tick");
  advance(1);
  take("event");
  reply(error("duplicate_conflict"));
  assert(gadget.connected() && gadget.queued() == 0 && gadget.lost() == 1);
  advance(1);
  take("event");
  assert(frame["name"] == "history_lost" && frame["data"]["dropped"] == 1);
  reply(error("unsupported_capability")); // A rejected report is dropped too.
  assert(gadget.connected() && gadget.lost() == 0);
  idlePoll();
  assert(gadget.connected());

  // Unknown failures reconnect with growing backoff, then drop after the attempt cap.
  gadget.emit("tick");
  uint32_t previous = 0;
  for (int attempt = 1; attempt <= grok::MaxEventAttempts; ++attempt) {
    advance(1);
    take("event");
    reply(error("mystery"));
    assert(!gadget.connected());
    if (attempt == grok::MaxEventAttempts)
      break;
    uint32_t waited = connect();
    assert(attempt == 1 || waited > previous || waited >= grok::MaxBackoffMs - 5);
    previous = waited;
  }
  assert(gadget.queued() == 0 && gadget.lost() == 1);
  connect();
  advance(1);
  take("event");
  assert(frame["name"] == "history_lost");
  reply(ok);
  idlePoll();
}

// ESP-09: a reply whose shape does not match the pending request resets the session.
void correlation() {
  gadget.emit("tick");
  advance(1);
  take("event");
  reply(hello);
  assert(!gadget.connected() && gadget.queued() == 1);
  connect();
  advance(1);
  take("event");
  reply(R"({"ok":true,"duplicate":true})");
  assert(gadget.connected() && gadget.queued() == 0);
  awaitPoll();
  reply(ok); // A state-shaped reply to a poll.
  assert(!gadget.connected());
  connect();
  // No timeout before the bridge's worst case has passed.
  next();
  advance(12000);
  assert(gadget.waiting() && gadget.connected());
  advance(1001);
  assert(!gadget.connected());
  connect();
}

// ESP-11: one JSON object per line, nothing else.
void strictLines() {
  for (const std::string &bad :
       {std::string(R"({"ok":true} x)"), std::string(R"({"ok":true}{"ok":false})"),
        std::string("{\"ok\":true}\0", 12), std::string("[true]"),
        std::string(R"({"ok":true,"extra":1})")}) {
    next();
    reply(bad);
    assert(!gadget.connected());
    connect();
  }
  awaitPoll();
  reply("{\"ok\":true,\"commands\":[]} \t\r");
  assert(gadget.connected());
}

// ESP-07: any frame the gateway may send is parsed, or answered with a failed ACK.
void largeCommands() {
  std::string head = R"({"command_id":"big-1","capability":"any.set","arguments":{"v":[)";
  std::string tail = "]}}";
  std::string items;
  while ((pollWith(head + items + "0" + tail)).size() + 3 <= grok::MaxFrame)
    items += "0,";
  items += "0";
  auto big = pollWith(head + items + tail);
  assert(big.size() + 1 <= grok::MaxFrame + 1);
  awaitPoll();
  int before = executions;
  reply(big);
  advance(1);
  take("ack");
  assert(frame["command_id"] == "big-1" && frame["status"] == "executed" &&
         executions == before + 1);
  reply(ok);

  std::string deep = R"({"command_id":"deep-1","capability":"any.set","arguments":)";
  for (int i = 0; i < 40; ++i)
    deep += R"({"a":)";
  deep += "0";
  for (int i = 0; i < 41; ++i)
    deep += "}";
  awaitPoll();
  reply(pollWith(deep));
  assert(gadget.connected());
  advance(1);
  take("ack");
  assert(frame["command_id"] == "deep-1" && frame["status"] == "failed" &&
         frame["error"]["code"] == "invalid_command" && executions == before + 1);
  reply(ok);
  assert(gadget.connected());
}

int main() {
  registration();
  Serial.output.clear();
  assert(gadget.command("any.set", any, nullptr, R"({"type":"object"})"));
  assert(gadget.event("tick"));
  assert(gadget.event("motion", R"({"properties":{"level":{"type":"integer"}}})"));
  assert(gadget.button(7));
  assert(gadget.begin() && Serial.rxBuffer == grok::SerialRxBuffer);
  fake::now += 500;
  gadget.loop();
  helloContents();
  emitRules();
  lossRetry();
  rejectedEvents();
  correlation();
  strictLines();
  largeCommands();
  std::cout << "session: registration limits, event schemas, stable loss IDs, rejected-event "
               "recovery, reply correlation, strict lines and full-size commands passed\n";
}
