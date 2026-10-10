# Use the library in another gadget

The C124 sketch and `examples/led-button` both use `grok::Gadget`. A second board still needs its own pins, a PlatformIO environment, and its own hardware check. This library has no Wi-Fi transport and no hosted service. The gateway device port stays on loopback. See the [hosting FAQ](https://github.com/adidshaft/grok-gadgets/blob/main/docs/getting-started/hosting.md).

| Header | Purpose |
| --- | --- |
| `GrokCore.h` | Bounded lines, queues, and button debounce |
| `GrokGadgets.h` | Capability table, RGB helpers, and ACK cache |
| `GrokSession.h` | `grok::Gadget`: hello, poll, events, loss reports, and recovery |
| `C124.h` | AtomS3 Lite pins (RGB GPIO35, button GPIO41) |

## Declare a global gadget

With the default eight-entry ACK cache, `grok::Gadget` holds about 40 KB (cache, line buffer, parse documents, and event queue). `grok::Device` alone is about 33 KB. `Device::execute` uses a 2 KB stack buffer. The Arduino `loop` task stack is 8 KB. Declare the gadget as a global or static object. A local with the default cache in `setup()` or `loop()` overflows that stack.

```cpp
#include <GrokSession.h>

bool on = false;
grok::Gadget gadget("Generic ESP32-S3 LED and button", "0.1.0", "esp32s3");

void report(JsonObject state, void *) { state["led"]["on"] = on; }
grok::Error setLed(JsonObjectConst args, void *) {
  if (args.size() != 1 || !args["on"].is<bool>())
    return {"invalid_arguments", "Expected {\"on\": boolean}"};
  on = args["on"];
  digitalWrite(4, on ? HIGH : LOW);
  return {};
}
void setup() {
  pinMode(4, OUTPUT);
  gadget.state(report);
  gadget.command("led.set", setLed, nullptr,
                 R"({"type":"object","properties":{"on":{"type":"boolean"}},)"
                 R"("required":["on"],"additionalProperties":false})");
  gadget.event("button.held", R"({"properties":{"ms":{"type":"integer"}}})");
  gadget.button(0);
  gadget.begin();
}
void loop() { gadget.loop(); }
```

`examples/led-button` is that sketch, with a one-second hold event. `pio run -e esp32s3-led-button` compiles it. The pins are a documentation example: LED GPIO4, BOOT button GPIO0. They are not a hardware measurement.

Strings passed to `command`, `event`, and `state` must outlive the gadget. Use literals. Call them before `begin()`.

## Names, events, and schemas

Hello may list at most 16 names. The gadget always appends `state` and `history_lost`. `button(pin)` appends `button`. Custom commands and events share the remaining slots. Registration returns false for a 17th name, a duplicate, or a reserved name (`button`, `state`, `history_lost`).

`event(name, schema)` declares a custom event. Its hello schema includes `"x-grok-gadgets-kind": "event"`. `history_lost` is reserved: the gadget emits it, and a sketch cannot register that name. `emit(name, data)` queues a declared event. The data object must be under 128 bytes. A full queue of 16 drops the new event and later reports `history_lost`.

`command(name, handler, context, schema)` registers a callable capability. The optional schema is an inline JSON object. Grok sees that object as the argument contract. `rgb.set` keeps the built-in RGB rules; a replacement schema is rejected.

The gateway rejects schema references, `pattern` and `patternProperties`. Use explicit properties, `enum`, `minLength` and `maxLength` instead. This prevents a device-supplied regular expression from blocking the gateway.

`begin()` calls `Serial.setRxBufferSize` for two full frames (4096 bytes) before `Serial.begin()`. The Arduino-ESP32 2.0.17 USB CDC queue otherwise stays 256 bytes and can drop a long reply. That call is unverified on hardware.

## Limits

A reply may be one JSON object of at most 2047 bytes plus LF. The parse document is sized for a gateway-legal frame (one value per two input bytes, plus the string bytes). A command that is still too deep gets a failed ACK (`invalid_command`) instead of a session reset. Trailing bytes, a second object, or an embedded NUL resets the session.

The request timeout is 13 seconds, longer than the USB bridge's connect-plus-reply wait. A reply whose shape does not match the outstanding request resets the session. Protocol 0.1.0 has no request id.

Within one boot the device retains eight acknowledgements by default. The same command id and the same compact arguments return the cached result and do not run the handler again. Changed arguments return `duplicate_conflict`. Reboot clears the cache. A timeout means the result is uncertain: do not repeat a physical action under a new id.

### ACK cache size

Set `GROK_ACK_CACHE_ENTRIES` to a positive integer at compile time. The default is `8`; zero does not disable the cache. Add `-DGROK_ACK_CACHE_ENTRIES=2` to your compiler flags for a two-entry cache. In PlatformIO, add this flag to the environment's `build_flags`, preserving its existing flags. Use the same value in every translation unit that includes the SDK headers.

Each entry stores a 65-byte ID and two 2048-byte buffers: 4161 bytes in total. Reducing eight entries to two saves 24,966 bytes of cache storage. Object sizes also include other members and alignment. The stack buffer size does not change.

Fewer entries retain fewer command IDs and therefore provide less replay protection. The cache replaces the oldest entry when it stores a new ID. Replaying a retained ID or reporting `duplicate_conflict` does not refresh or evict entries. Once an ID is evicted, receiving that command again can run its handler again. Successful and failed results use the same cache.

`readRgb` accepts integer `r`, `g`, and `b` from 0 to 255, plus boolean `on`. A float such as `255.0` is rejected. The gateway's own numeric check is a separate code path.

## License

Original SDK code is Apache-2.0. Read the [dependency notices](dependencies.md) before distributing firmware.
