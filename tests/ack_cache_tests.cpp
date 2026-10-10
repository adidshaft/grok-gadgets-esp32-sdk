// SPDX-License-Identifier: Apache-2.0
#undef NDEBUG // Tests are assertions.
#include "GrokGadgets.h"
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

struct App {
  int executions = 0;
};
void state(JsonObject target, void *raw) {
  target["executions"] = static_cast<App *>(raw)->executions;
}
grok::Error succeed(JsonObjectConst, void *raw) {
  ++static_cast<App *>(raw)->executions;
  return {};
}
grok::Error fail(JsonObjectConst, void *raw) {
  ++static_cast<App *>(raw)->executions;
  return {"handler_failed", "Expected test failure"};
}
std::string jsonText(const JsonDocument &document) {
  std::string text;
  serializeJson(document, text);
  return text;
}
void command(JsonDocument &document, int index) {
  document.clear();
  document["command_id"] = "small-" + std::to_string(index);
  document["capability"] = index % 2 ? "counter.fail" : "counter.bump";
  document.createNestedObject("arguments");
}
int main() {
  // Both small configurations must save RAM, not only change the eviction limit.
  static_assert(sizeof(grok::Device) < 16 * 1024, "Small cache must fit below 16 KB");
  App app;
  grok::Device device(state, &app);
  assert(device.capability("counter.bump", succeed, &app));
  assert(device.capability("counter.fail", fail, &app));
  DynamicJsonDocument input(4096), ack(4096);
  std::vector<std::string> originals;
  for (int index = 0; index < GROK_ACK_CACHE_ENTRIES; ++index) {
    command(input, index);
    device.execute(input.as<JsonObjectConst>(), ack);
    assert(ack["status"] == (index % 2 ? "failed" : "executed"));
    originals.push_back(jsonText(ack));
  }
  assert(app.executions == GROK_ACK_CACHE_ENTRIES);

  // Repeated reads preserve the original ACK and do not advance FIFO eviction.
  for (int retry = 0; retry < 4; ++retry)
    for (int index = 0; index < GROK_ACK_CACHE_ENTRIES; ++index) {
      command(input, index);
      device.execute(input.as<JsonObjectConst>(), ack);
      assert(jsonText(ack) == originals[index]);
      assert(app.executions == GROK_ACK_CACHE_ENTRIES);
    }
  command(input, 0);
  input["arguments"]["changed"] = true;
  device.execute(input.as<JsonObjectConst>(), ack);
  assert(ack["error"]["code"] == "duplicate_conflict");
  assert(app.executions == GROK_ACK_CACHE_ENTRIES);
  command(input, 0);
  device.execute(input.as<JsonObjectConst>(), ack);
  assert(jsonText(ack) == originals[0]);

  // The next new ID evicts the oldest, while other retained IDs remain protected.
  command(input, GROK_ACK_CACHE_ENTRIES);
  device.execute(input.as<JsonObjectConst>(), ack);
  assert(app.executions == GROK_ACK_CACHE_ENTRIES + 1);
  const auto newest = jsonText(ack);
  device.execute(input.as<JsonObjectConst>(), ack);
  assert(jsonText(ack) == newest);
  assert(app.executions == GROK_ACK_CACHE_ENTRIES + 1);
  for (int index = 1; index < GROK_ACK_CACHE_ENTRIES; ++index) {
    command(input, index);
    device.execute(input.as<JsonObjectConst>(), ack);
    assert(jsonText(ack) == originals[index]);
    assert(app.executions == GROK_ACK_CACHE_ENTRIES + 1);
  }
  command(input, 0);
  device.execute(input.as<JsonObjectConst>(), ack);
  assert(app.executions == GROK_ACK_CACHE_ENTRIES + 2);

  // Exercise repeated wraparound, including replay of failed handler results.
  for (int index = GROK_ACK_CACHE_ENTRIES + 1; index < GROK_ACK_CACHE_ENTRIES + 9; ++index) {
    const int before = app.executions;
    command(input, index);
    device.execute(input.as<JsonObjectConst>(), ack);
    assert(app.executions == before + 1);
    assert(ack["status"] == (index % 2 ? "failed" : "executed"));
    const auto result = jsonText(ack);
    for (int retry = 0; retry < 4; ++retry) {
      device.execute(input.as<JsonObjectConst>(), ack);
      assert(jsonText(ack) == result);
      assert(app.executions == before + 1);
    }
  }
  grok::Device rebooted(state, &app);
  assert(rebooted.capability("counter.bump", succeed, &app));
  assert(rebooted.capability("counter.fail", fail, &app));
  const int before = app.executions;
  rebooted.execute(input.as<JsonObjectConst>(), ack);
  assert(app.executions == before + 1);
  std::cout << "ACK cache: " << GROK_ACK_CACHE_ENTRIES
            << " entries; replay, conflicts, FIFO eviction, wraparound and reboot pass\n";
}
