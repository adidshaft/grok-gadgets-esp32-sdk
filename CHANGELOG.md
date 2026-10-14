# Changelog

## Unreleased

- Setup hotspot with a captive page ([#114](https://github.com/adidshaft/grok-gadgets-esp32-sdk/issues/114), hub ADR 010). `GrokSetup.h`: a board with a factory identity and no home Wi-Fi opens `GrokGadget-XXXX` with the label's WPA2 passphrase, for one phone, for 10 minutes. The page at `192.168.4.1` (captive DNS; every other path redirects there) asks for the label's setup code, lists 2.4 GHz networks, joins the chosen one and shows live status: joining, the join failure reason, joined, Online. Wrong codes: 5 free, then a doubling 1-minute lock; 20 close the hotspot. The page never shows a stored value. After a join the board saves only the home Wi-Fi, connects to the relay with the factory identity, and closes the hotspot 30 s after Online. The OLED kit shows `Setup N min` and the hotspot name, then `Joining WiFi` or the reason. `WifiLink::begin` can keep the hotspot up. Wi-Fi build: static RAM 108,076 B and flash 932,549 B (+8.5 KB RAM and +45.4 KB flash against 0.2.0-alpha.14). Host-tested and compiled; no kit has run it yet.

- Factory identity separate from user settings ([#115](https://github.com/adidshaft/grok-gadgets-esp32-sdk/issues/115), hub ADR 010). NVS namespace `grok-factory` holds the device token, relay name and port, CA, setup-hotspot passphrase and setup code. The console command `factory_provision` writes it once (`replace` to change it) and never echoes a value. `grok-wifi` holds the home Wi-Fi. A BOOT reset or `factory_reset` erases `grok-wifi` only. A full `provision` line and boards provisioned before this change keep working. `status` reports `factory`. Static RAM +6.9 KB and flash +2.6 KB on the Wi-Fi build. Host-tested and compiled; no kit has run it yet.

## 0.2.0-alpha.14 — 14 October 2026

- On 13 October 2026 the owner's kit, on this firmware, reached a gateway relay on an always-on Linux server by DNS name. Grok Bot on the iPhone, on cellular, showed text on it while the Mac slept. A wrong CA showed `TLS rejected`; revoke showed `Token rejected` ([hub record](https://github.com/adidshaft/grok-gadgets/blob/dev/docs/verification/mac-free-route-2026-10-13.md)).
- A relay's DNS name for the Wi-Fi gateway ([#111](https://github.com/adidshaft/grok-gadgets-esp32-sdk/issues/111), hub ADR 009). `provision` accepts a public DNS name such as `devices.example.net` (lower case, two or more labels, up to 63 characters) beside an IPv4 address and a `.local` name. `WifiLink` looks the name up before each connection and checks the certificate against it. The OLED kit's top row shows `Relay...` while it connects and `Relay name?` if the name does not resolve. Host-tested and compiled; no kit has run it yet.
- Reconnect waits get jitter: each is cut by up to a quarter at random, never below 0.5 s, so boards that lost the same relay spread out.
- After a Wi-Fi drop, the join timer starts again, so the status shows `WiFi...` and not `Join timeout` at the first retry.

## 0.2.0-alpha.13 — 13 October 2026

- Documentation only; the library, firmware and examples are unchanged from 0.2.0-alpha.12. README, the first experiment, protocol recovery and SECURITY record that Grok Bot called the gateway's tools natively from the Mac and the iPhone and showed text on the OLED kit on 13 October 2026, through an owner-run tunnel to the MCP port only (#106). The install steps use the published packages (#105).

## 0.2.0-alpha.12 — 13 October 2026

- Wi-Fi build: the gateway can be provisioned as an mDNS name such as `my-mac.local`. Before each connection the board resolves it with `ESPmDNS` and verifies the TLS certificate against the name, so a new computer address needs no new settings. An unanswered name shows `Gateway name?`. IPv4 settings keep working. On 12–13 October 2026 the owner's kit connected by name and followed two Mac address changes and a Mac restart without new settings ([gateway record](https://github.com/adidshaft/grok-gadgets-gateway/blob/main/docs/verification/always-on-2026-10-13.md)) ([#99](https://github.com/adidshaft/grok-gadgets-esp32-sdk/issues/99), hub ADR 006).

## 0.2.0-alpha.11 — 12 October 2026

- OLED Wi-Fi build: while `Online`, show a 48×48 Grok Bot face on the right of the screen and `Grok Bot` on the second row. The face moves one pixel and blinks every 3.6 s. Message text uses the bottom two rows. Connection errors and reset messages replace the face and label ([#91](https://github.com/adidshaft/grok-gadgets-esp32-sdk/issues/91), [#93](https://github.com/adidshaft/grok-gadgets-esp32-sdk/issues/93)). The owner checked the 48×48 face and label on the kit on 12 October 2026; text on rows 6–7 is host-tested only.

## 0.2.0-alpha.10 — 12 October 2026

- Wi-Fi build: when the board cannot join, the OLED top row says why (`No network`, `Wrong password`, `Join timeout`) and the second row says what to check (`2.4GHz? Name?`, `Check password`, `Check router`). The console prints the same cause, and the `status` reply adds `wifi_failure` and `wifi_hint`. Neither shows the network name or password ([#73](https://github.com/adidshaft/grok-gadgets-esp32-sdk/issues/73)).
- Fix: every `provision` console line crashed the Wi-Fi build after the change above (`Stack canary watchpoint triggered (loopTask)`). Two 3.4 KB settings copies sat on the 8 KB loop stack; the console now uses one buffer outside the stack. `v0.2.0-alpha.9` was within about 500 bytes of this limit, and no board has been provisioned with it.
- The wrong-password message and the provisioning fix passed on the owner's kit on 12 October 2026 ([record](docs/verification/wifi-join-failure-2026-10-12.md)). `No network` and `Join timeout` are host-tested only.

## 0.2.0-alpha.9 — 12 October 2026

- Wi-Fi build: after BOOT is held for 3 s, the status row counts down to the 10 s factory reset (`Reset in 7s` … `Reset in 1s`). Letting go keeps the settings. In the Wi-Fi acceptance the owner reset the board by accident while waiting for feedback (#80). The countdown passed a physical check on the owner's kit (`docs/verification/oled-kit-wifi-2026-10-12.md`).

## 0.2.0-alpha.8 — 12 October 2026

- The OLED kit passed physical acceptance over Wi-Fi on the owner's board on 12 October 2026: connect on boot, text and clear, `BOOT` events, Wi-Fi loss and recovery (after the rejoin fix), gateway restart, revoke and re-enroll, wrong-CA rejection and power cycle. Grok Bot showed text on it over Wi-Fi through local shell execution. See `docs/verification/oled-kit-wifi-2026-10-12.md` (#5).
- Wi-Fi build: while offline, start a fresh Wi-Fi join every 15 s. The core's auto-reconnect stopped retrying after the router was off for minutes in the Phase 4 physical test (#5).
- Wi-Fi transport (hub ADR 005, ESP-005, #5). `grok::Link` lets a gadget own its connection: the session opens it before each hello, adds the device token to the hello and closes it on every reset. After `unauthorized` or `revoked` the next hello waits 60 s. `Gadget::rejected()` reports the code. Frames are written with one `write` call, so a TLS link sends one record per frame. USB behaviour and frames are unchanged.
- `GrokWifi.h`: a TLS link that trusts only the provisioned gateway CA, Wi-Fi settings in NVS namespace `grok-wifi`, and a one-line JSON serial console (`status`, `provision`, `factory_reset`) that never prints a stored value.
- `examples/oled-kit` gains the `esp32s3-oled-kit-wifi` build: a status row, provisioning over USB with the gateway's `wifi-provision`, and a 10-second BOOT hold as a factory reset. Physical acceptance over Wi-Fi is pending.
- Host tests `wifi_console` and `wifi_link`, and the `transcript_oled_kit_wifi` contract transcript. CI compiles the Wi-Fi build.

## 0.2.0-alpha.7 — 12 October 2026

- `examples/oled-kit`: firmware for the ESP32-S3-N16R8 board (YD-ESP32-23 v1.3) with a 1.3-inch SH1106 I²C OLED. Commands `display.text`, `display.clear` and `rgb.set`, plus `BOOT` button events. It finds the display at 0x3c or 0x3d with either data-pin order and reports the pins and address. Build it with the new `esp32s3-oled-kit` environment (16 MB flash, octal PSRAM, U8g2 2.36.18) (#65).
- The kit passed physical acceptance on the owner's board on 12 October 2026: text, clear, rejected text, RGB light, `BOOT` events, unplug/replug and gateway restart. Grok Bot showed text on it through local shell execution. See `docs/verification/oled-kit-2026-10-12.md` (#65, #66).

## 0.2.0-alpha.6 — 12 October 2026

- `GROK_ACK_CACHE_ENTRIES` sets the number of cached ACK entries at compile time. The default stays 8; zero or a negative value fails to compile. Host tests cover one-entry and two-entry caches (#59, contributed by @Hellz0wnzJ00).
- Docs: the ordered ESP32-S3 and OLED kit is the first physical experiment, with a software rehearsal (#60).

## 0.2.0-alpha.5 — 8 October 2026

- `.gitignore` ignores `dist` as a file as well as a folder, so a stray `pio pkg pack -o dist` output no longer shows as untracked.

## 0.2.0-alpha.4 — 8 October 2026

- Tooling: PlatformIO Core 6.2.0, `clang-format` 23.1.2, `ruff` 0.16.10 and `jsonschema` 4.26.0. `requirements.lock` is regenerated from these pins. Sources need no reformatting; firmware and library are unchanged.

## 0.2.0-alpha.3 — 8 October 2026

- Dependabot opens weekly grouped update PRs into `dev`.
- CONTRIBUTING starts fork branches from `upstream/dev`. CI also runs the plain-language check on CONTRIBUTING and SUPPORT.

## 0.2.0-alpha.2 — 7 October 2026

- Clarify Grok Bot as the sole assistant target and document the local rehearsal path.
- Retain the existing C124 firmware and library package version; no protocol change.

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
