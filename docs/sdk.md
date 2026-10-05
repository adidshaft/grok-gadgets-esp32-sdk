# Use the library in another gadget

The C124 example uses the reusable SDK. It does not define the SDK's board support limit.

| Library | Purpose | Dependencies |
| --- | --- | --- |
| `GrokCore.h` | Bounded lines, queues and button debounce | No Arduino dependency |
| `GrokGadgets.h` | Device capabilities and command results | ArduinoJson 6.21.5; no board or serial dependency |

## Add the library

1. Add `lib/GrokGadgets` to your PlatformIO project.
2. Pin ArduinoJson 6.21.5.
3. Include `GrokGadgets.h`.
4. Implement hardware handlers, state reporting and transport authentication in your application.

This counter example needs no RGB device:

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

## Register capabilities

Names, handlers and context storage must remain valid for the lifetime of `Device`. Register no more than 16 unique names.

Include `gadget.capabilities()` in the hello message. Also include read-only capabilities, such as button and state.

Each handler validates its own JSON arguments. Return an empty `Error` after execution. On failure, return a static error code and message.

`readRgb` accepts exactly integer `r`, `g` and `b` values from 0 to 255, plus a boolean `on` value. It validates a temporary value before hardware changes.

## Keep state within limits

`StateWriter` writes the state object. Give the application JSON documents at least 4096 bytes of capacity. Keep each serialized acknowledgement or state frame below or equal to 2048 bytes, including LF.

If the state callback exceeds the document or frame limit, `Device` keeps the execution status but returns an empty state object. Empty state means unknown state. The SDK caches this bounded result to protect retries.

Check document overflow before transmission. The fallback does not remove the 4096-byte document-capacity requirement.

## Handle retries

Within one boot, `Device` retains eight acknowledgements and their exact compact command envelopes. A retained duplicate returns the original result without running the handler again.

Changed parameters with the same ID produce `duplicate_conflict`. Different JSON key order also counts as changed parameters.

Eviction or reboot ends this protection. This is not durable exactly-once execution. The gateway does not replay an old dispatched command into a new session. A timeout means the result is uncertain. Never resend an uncertain physical action with a new ID.

On retry, the SDK decodes the cached result into the destination document. It does not change the stored result. Decoded strings belong to the destination document.

If decoding fails, the SDK returns `failed` with `ack_unavailable` and empty state. It does not run the handler. This error does not prove the original action failed.

The cached result remains available. Retry with sufficient document capacity. A retry or conflict does not extend the eight-entry FIFO retention window.

## License

Original SDK code uses Apache-2.0. Read the [dependency notices](dependencies.md) before you distribute firmware.
