// SPDX-License-Identifier: Apache-2.0
#undef NDEBUG // Tests are assertions.
#include "GrokGadgets.h"
#include <cassert>
#include <iostream>
struct App {
  grok::Rgb rgb;
  int executions = 0;
};
void state(JsonObject target, void *raw) { grok::writeRgb(target, static_cast<App *>(raw)->rgb); }
grok::Error rgb(JsonObjectConst args, void *raw) {
  auto &app = *static_cast<App *>(raw);
  grok::Rgb next;
  auto e = grok::readRgb(args, next);
  if (!e.code) {
    app.rgb = next;
    ++app.executions;
  }
  return e;
}
grok::Error counter(JsonObjectConst args, void *raw) {
  if (args.size() != 0)
    return {"invalid_arguments", "No args"};
  ++*static_cast<int *>(raw);
  return {};
}
void largeState(JsonObject target, void *) { target["padding"] = std::string(3000, 'x'); }
void retainedState(JsonObject target, void *) { target["padding"] = std::string(1000, 'x'); }
grok::Error fail(JsonObjectConst, void *raw) {
  ++*static_cast<int *>(raw);
  return {"handler_failed", "Expected test failure"};
}
std::string jsonText(const JsonDocument &document) {
  std::string json;
  serializeJson(document, json);
  return json;
}
void retryTests() {
  App app;
  int successes = 0, failures = 0;
  grok::Device device(state, &app);
  assert(device.capability("counter.bump", counter, &successes));
  assert(device.capability("counter.fail", fail, &failures));
  DynamicJsonDocument success(4096), failure(4096), ack(4096);
  assert(!deserializeJson(
      success, R"({"command_id":"repeat-success","capability":"counter.bump","arguments":{}})"));
  assert(!deserializeJson(
      failure, R"({"command_id":"repeat-failure","capability":"counter.fail","arguments":{}})"));
  device.execute(success.as<JsonObjectConst>(), ack);
  auto successAck = jsonText(ack);
  device.execute(failure.as<JsonObjectConst>(), ack);
  auto failureAck = jsonText(ack);
  assert(ack["status"] == "failed" && ack["error"]["code"] == "handler_failed");
  // Interleave retained success/failure IDs through four retries each.
  for (int retry = 0; retry < 4; ++retry) {
    app.rgb.r = retry + 1; // Replay must preserve the original state snapshot.
    device.execute(success.as<JsonObjectConst>(), ack);
    assert(ack.is<JsonObject>() && jsonText(ack) == successAck);
    device.execute(failure.as<JsonObjectConst>(), ack);
    assert(ack.is<JsonObject>() && jsonText(ack) == failureAck);
    assert(successes == 1 && failures == 1);
  }
  success["arguments"]["changed"] = true;
  for (int retry = 0; retry < 4; ++retry) {
    device.execute(success.as<JsonObjectConst>(), ack);
    assert(ack["error"]["code"] == "duplicate_conflict" && successes == 1);
  }
  success["arguments"].remove("changed");
  device.execute(success.as<JsonObjectConst>(), ack);
  assert(jsonText(ack) == successAck && successes == 1);
  // Returned replay owns its decoded strings; clearing the input cannot damage it.
  success.clear();
  assert(jsonText(ack) == successAck);

  // A valid cached frame can exceed a caller's destination capacity. Fail safely,
  // retain the command, then recover the original ACK with an adequate document.
  int retainedExecutions = 0;
  grok::Device retained(retainedState, nullptr);
  assert(retained.capability("counter.bump", counter, &retainedExecutions));
  assert(!deserializeJson(
      success, R"({"command_id":"large-cache","capability":"counter.bump","arguments":{}})"));
  retained.execute(success.as<JsonObjectConst>(), ack);
  auto original = jsonText(ack);
  assert(ack["status"] == "executed" && ack["state"]["padding"].as<std::string>().size() == 1000);
  DynamicJsonDocument smallAck(512);
  for (int retry = 0; retry < 4; ++retry) {
    retained.execute(success.as<JsonObjectConst>(), smallAck);
    assert(!smallAck.overflowed() && smallAck["status"] == "failed" &&
           smallAck["error"]["code"] == "ack_unavailable" && retainedExecutions == 1);
  }
  retained.execute(success.as<JsonObjectConst>(), ack);
  assert(jsonText(ack) == original && retainedExecutions == 1);

  // Exactly eight distinct IDs are retained. Reading an ACK does not extend FIFO
  // retention; the ninth distinct command permits the evicted ID to run again.
  int executions = 0;
  grok::Device bounded(state, &app);
  assert(bounded.capability("counter.bump", counter, &executions));
  for (int index = 0; index < 8; ++index) {
    success["command_id"] = std::string("bounded-") + std::to_string(index);
    bounded.execute(success.as<JsonObjectConst>(), ack);
  }
  success["command_id"] = "bounded-0";
  bounded.execute(success.as<JsonObjectConst>(), ack);
  assert(executions == 8);
  success["command_id"] = "bounded-8";
  bounded.execute(success.as<JsonObjectConst>(), ack);
  success["command_id"] = "bounded-1";
  bounded.execute(success.as<JsonObjectConst>(), ack);
  assert(executions == 9); // Last retained boundary remains protected.
  success["command_id"] = "bounded-0";
  bounded.execute(success.as<JsonObjectConst>(), ack);
  assert(executions == 10);
  grok::Device rebooted(state, &app);
  assert(rebooted.capability("counter.bump", counter, &executions));
  rebooted.execute(success.as<JsonObjectConst>(), ack);
  assert(executions == 11 && ack["status"] == "executed");
}
int main() {
  retryTests();
  App app;
  int count = 0;
  grok::Device device(state, &app);
  assert(device.capability("rgb.set", rgb, &app));
  assert(!device.capability("rgb.set", rgb, &app));
  assert(device.capability("counter.bump", counter, &count));
  assert(!device.capability("bad name", counter, &count));
  for (const char *reserved : {"button", "state", "history_lost"})
    assert(!device.capability(reserved, counter, &count));
  DynamicJsonDocument cmd(2048), ack(2048);
  deserializeJson(
      cmd,
      R"({"command_id":"cmd-1","capability":"rgb.set","arguments":{"r":0,"g":255,"b":0,"on":true}})");
  device.execute(cmd.as<JsonObjectConst>(), ack);
  assert(ack["status"] == "executed");
  assert(app.rgb.g == 255 && app.executions == 1);
  device.execute(cmd.as<JsonObjectConst>(), ack);
  assert(ack["status"] == "executed" && app.executions == 1);
  cmd["arguments"]["r"] = 3;
  device.execute(cmd.as<JsonObjectConst>(), ack);
  assert(ack["error"]["code"] == "duplicate_conflict");
  assert(app.executions == 1);
  for (auto invalid : {"-1", "256", "1.5", "true", "\"4\""}) {
    std::string json =
        std::string(
            "{\"command_id\":\"temporary\",\"capability\":\"rgb.set\",\"arguments\":{\"r\":") +
        invalid + ",\"g\":0,\"b\":0,\"on\":true}}";
    // Each invalid channel has an independent command ID.
    assert(!deserializeJson(cmd, json));
    cmd["command_id"] = std::string("invalid-") + std::to_string(count++);
    device.execute(cmd.as<JsonObjectConst>(), ack);
    assert(ack["status"] == "failed");
    assert(app.executions == 1);
  }
  deserializeJson(cmd, R"({"command_id":"custom-1","capability":"counter.bump","arguments":{}})");
  int before = count;
  device.execute(cmd.as<JsonObjectConst>(), ack);
  assert(ack["status"] == "executed" && count == before + 1);
  cmd["command_id"] = "custom-2";
  cmd["capability"] = "unknown";
  device.execute(cmd.as<JsonObjectConst>(), ack);
  assert(ack["error"]["code"] == "unsupported_capability");
  cmd["command_id"] = "invalid id";
  device.execute(cmd.as<JsonObjectConst>(), ack);
  assert(ack["error"]["code"] == "invalid_command");
  grok::Device large(largeState, nullptr);
  int largeCount = 0;
  large.capability("counter.bump", counter, &largeCount);
  deserializeJson(cmd, R"({"command_id":"large-1","capability":"counter.bump","arguments":{}})");
  DynamicJsonDocument largeAck(8192);
  large.execute(cmd.as<JsonObjectConst>(), largeAck);
  assert(largeAck["status"] == "executed" && largeAck["state"].size() == 0 &&
         measureJson(largeAck) <= grok::MaxFrame);
  large.execute(cmd.as<JsonObjectConst>(), largeAck);
  assert(largeCount == 1 && largeAck["status"] == "executed");
  std::cout << "device: RGB bounds/types, extension handler, malformed IDs, unsupported capability "
               "and repeated success/failure ACKs, interleaving/conflict, decode recovery, "
               "eight-ID eviction and reboot passed\n";
}
