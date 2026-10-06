# Changelog

## Unreleased

## 0.2.0-alpha.1 — 6 October 2026

- Prepare PlatformIO and Arduino Library Manager packaging (not published): fuller `library.json`, a new `library.properties`, LICENSE and NOTICE inside the library, and `tools/check_package.py` in CI.
- Protocol README re-pinned from gateway `3e41aec`: capability `description` and 16 KiB TCP hellos are documented. USB firmware keeps its 2048-byte frames; wire format unchanged.
- `late_ack` and `unknown_command` replies to an ACK no longer reset the session: the firmware drops that ACK and keeps polling. New `droppedAcks()` counter. Protocol README re-pinned from gateway `c568c3e` (wire format unchanged).
- CI runs the README Quickstart in a fresh checkout and the simulated-firmware USB integration against gateway `main`, on push, pull request and nightly.
- New `grok::Gadget` (`GrokSession.h`): declare commands, events and a button, then call `begin()` and `loop()`. The C124 firmware is now a short sketch; `examples/led-button` is a second, generic ESP32-S3 sketch.
- Custom events with optional data schemas, sent with `"x-grok-gadgets-kind": "event"` in `capability_schemas`.
- `history_lost` retries reuse one event ID and count. Rejected events are dropped and counted instead of blocking polls. Backoff resets only after real traffic.
- Any reply frame up to the 2048-byte limit parses; too-deep commands get a failed ACK. Replies must be one JSON object of the expected shape. Request timeout is 13 s, longer than the bridge's worst case.
- Registration rejects reserved names (`button`, `state`, `history_lost`), duplicates and more than 16 hello names.
- Device ID prints MAC bytes in order (`c124-<mac>`). Firmware version 0.2.0.
- The contract check validates frames written by the real sketches.
- Hello schemas for custom events include `"x-grok-gadgets-kind": "event"`. `history_lost` stays reserved.
- `Serial.setRxBufferSize` runs before `Serial.begin()` (unverified on hardware). `invalid_state` drops a queued event instead of stopping polls.
- Firmware hashes depend on the Xtensa toolchain host variant. `-ffile-prefix-map` keeps checkout paths out of the image. The only current checksum record is `docs/build-checksums.json`.
- Local packages include `boot_app0.bin` and PlatformIO flash offsets. USB unplug stops the bridge; start it again after the board power-cycles.

## 0.1.0 (unreleased local alpha)

- Portable C++14 bounded framing, queue and button debounce utilities.
- Extensible ArduinoJson capability handlers and state callbacks; strict RGB validation.
- Eight-command ACK retention, duplicate conflict detection and bounded ACK fallback.
- Standalone AtomS3 Lite C124 USB firmware with RGB/button, periodic reported state, event-loss signals and session recovery.
- Pinned PlatformIO/Arduino dependencies and generated frame contract checks.
- Host simulation of actual firmware, PTY bridge/gateway acceptance, real ESP32-S3 firmware compilation and local binary checksums.
- Declared history_lost capability and actual consumer 20-edge queue-overflow acceptance, preserving the gateway session and later commands.
- Flash/recovery instructions, structured issues and inactive CI/protection proposals.

No physical board, Grok client, Wi-Fi transport, public release or independently reproduced installation is verified. Original code is Apache-2.0; binary redistribution awaits dependency notice/source review.

## Local hardening (unpublished)

- Preserve retained ACKs over repeated retries using read-only ArduinoJson parsing; report replay decode failures without rerunning the handler.
- Cover interleaved successful/failed commands, conflict, eight-entry FIFO eviction, reboot and actual consumer/PTY retries. Firmware provenance packaging rebuilds a clean identified source and records the resolved toolchain.
