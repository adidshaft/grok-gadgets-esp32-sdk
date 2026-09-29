# Using the library in another gadget

The C124 example is a library consumer. Reuse `GrokCore.h` without Arduino for bounded lines, queues and button debounce. `GrokGadgets.h` adds ArduinoJson 6.21.5 and an extensible device capability registry, with no board or serial dependency. Add `lib/GrokGadgets` to another PlatformIO project, pin ArduinoJson, and include the header. The consumer owns hardware, authentication/transport adapters and state.

```cpp
#include <GrokGadgets.h>
int counter = 0;
void state(JsonObject out, void*) { out["counter"] = counter; }
grok::Error bump(JsonObjectConst args, void*) {
  if (args.size()) return {"invalid_arguments", "Expected empty arguments"};
  ++counter;
  return {};
}
grok::Device gadget(state, nullptr);
// In setup:
gadget.capability("counter.bump", bump, nullptr);
// Once a canonical poll command arrives:
// gadget.execute(command.as<JsonObjectConst>(), acknowledgement);
```

Names and handler/context storage must outlive Device. Publish `gadget.capabilities()` in hello alongside read-only names like button/state. Register at most 16 unique names. Handlers validate their own JSON arguments; return an empty Error for execution or a static code/message on failure. StateWriter writes a bounded state object. The application must size JsonDocuments sufficiently (4096 bytes in the consumer) and keep serialized ACK/state below 2048 including LF. Check document overflow before transmitting. `readRgb` strictly accepts exactly integer r/g/b 0..255 and a boolean on; it validates into a temporary value before touching hardware.

Device retains eight ACKs and exact compact serialized command envelopes within one boot. A retained duplicate returns the original execution state without rerunning its handler. Changed parameters under the same ID fail with duplicate_conflict. Different JSON key ordering is conservatively treated as different parameters. Eviction and reboot end that window; this is not durable exactly-once execution. Gateway never replays an old dispatched command into a new session. A timeout is uncertainty, never permission to resend an action with a new ID.

Original SDK code is Apache-2.0. See dependency notices before distributing your firmware. The SDK is useful independently with custom capabilities; tests include a non-RGB counter handler.
