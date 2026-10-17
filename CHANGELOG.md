# Changelog

## Unreleased

- Three builder examples on a new `grok::WifiApp` ([#127](https://github.com/adidshaft/grok-gadgets-esp32-sdk/issues/127), [#128](https://github.com/adidshaft/grok-gadgets-esp32-sdk/issues/128), [#129](https://github.com/adidshaft/grok-gadgets-esp32-sdk/issues/129), hub ADR 013). `GrokApp.h` gives a gadget without a display the relay link, phone setup, claim codes, signed updates (with a key), the USB console and the BOOT factory reset in two calls. `climate-sensor` (AHT20: `climate` state, `threshold.crossed`, `threshold.set`), `switch` (low-voltage relay module: `switch.set`, off at boot and after 30 s without the relay) and `servo` (SG90: `servo.move`, acknowledged when the move ends). Each has a host model, a contract transcript and a CI build (`esp32s3-climate-sensor`, `-switch`, `-servo`). Not tested on hardware yet.
- The setup page shows the claim code after the gadget goes Online, for gadgets without a display (hub ADR 011 §10), and then stays up 3 minutes so the person can read it. The USB console never prints it.
- Guide: [Ship a gadget](docs/ship-a-gadget.md): build with the update key, flash, read IDs, batch, factory-provision ([#118](https://github.com/adidshaft/grok-gadgets-esp32-sdk/issues/118)).

## 0.2.0-alpha.17 — 17 October 2026

- On 17 October 2026 the owner's kit took signed updates from the owner's relay with no cable. The OLED showed `Update %`, the kit restarted and reported the new version in its hello. It refused an image signed with another key before any download, and an image with one changed byte at the SHA-256 check. An image that never came Online reverted by itself after 5 minutes and showed `Reverted`. A power cut during a download recovered ([hub record](https://github.com/adidshaft/grok-gadgets/blob/dev/docs/verification/ota-2026-10-17.md)). The first USB flash of this firmware came Online against a relay that refuses the new hello field.

- The OLED stays usable across update downloads and restarts ([#148](https://github.com/adidshaft/grok-gadgets-esp32-sdk/issues/148)). In the acceptance the kit's display went dark after a download while the board kept running. The firmware now clears the I2C bus before the boot probe and sets the OLED up again after each download. The console `status` reports `display`, and its reply buffers grew so that every field fits.

- `OLED_KIT_TEST_NEVER_ONLINE` builds a signed test image that never connects, to show a revert (acceptance only; never ship it).

- Signed updates over the relay ([#125](https://github.com/adidshaft/grok-gadgets-esp32-sdk/issues/125), hub ADR 012). `GrokUpdate.h` adds `grok::Updater`: with a builder's public key compiled in, the gadget sends `"update": "ready"` in its hello, checks the ECDSA P-256 signature and every manifest field before downloading, writes 1,024-byte chunks into the other app slot while still polling once a second, and checks the size and SHA-256 before `Update.end()`. A new image starts on trial and marks itself valid only after a hello and a poll through the relay; after 5 minutes without that, or after a crash, the previous image starts, reports `reverted` and refuses that version. `GrokUpdateEsp.h` holds the ESP32 parts (`Update`, app rollback, NVS namespace `grok-update`, mbedTLS, up to two keys). `Gadget::updates()` attaches it; an older relay refuses the hello field once and the gadget asks no more until the next boot. The OLED kit shows `Update 42%` and `Reverted`, reports a real version (`0.2.0-alpha.17`, or `GROK_FIRMWARE_VERSION`) instead of `0.2.0-wifi`, and compiles a key in from `GROK_UPDATE_PUBLIC_KEY` (`tools/update_build.py`); without a key it never asks. New compile check `esp32s3-oled-kit-wifi-updates`. Host tests use real ECDSA and SHA-256 against the gateway's pinned update fixture (protocol pinned at gateway `7dd5b3a`). Guide: [signed updates](docs/updates.md).
- ESP-CORE-001: keep Arduino-ESP32 2.0.17 for Phase 12b; the [core record](docs/core-3x-decision.md) compares 3.3.12 (owner decision, 17 October 2026).

## 0.2.0-alpha.16 — 17 October 2026

- On 17 October 2026 the owner's kit, on this firmware, showed `Code XXXX-XXXX` while unclaimed on the owner's relay in accounts mode. A second person claimed it with that code from their own Grok Bot and showed text on it. After a BOOT reset and a phone re-join it showed a transfer code, and the owner's claim with that code stayed pending until the second person removed it ([hub record](https://github.com/adidshaft/grok-gadgets/blob/dev/docs/verification/claim-2026-10-17.md)). Right after the flash it reconnected to the relay and reported `claimed: true`: the migration had given it to the operator account.

- Claim codes ([#116](https://github.com/adidshaft/grok-gadgets-esp32-sdk/issues/116), hub ADR 011). `Gadget::showClaimCodes()` before `begin()` adds `"claim": "ready"` to the hello and accepts `claimed` and `claim_code` in hello and poll replies; every other unknown reply field still resets the session. `claimCode()` and `claimed()` read them; a disconnect clears the code. `claimAfterSetup()` makes the next accepted hello say `after_setup`. A relay that refuses the field (`invalid_request`) gets a hello without it until the next boot. The OLED Wi-Fi build opts in, shows `Code XXXX-XXXX` and `Add in Grok Bot` while a code is live, marks the first hello after a phone setup, and reports `claimed` in the console `status` (never the code). Protocol files follow gateway `dev`. Wi-Fi build: static RAM 108,108 B and flash 933,393 B (+32 B RAM, +844 B flash against 0.2.0-alpha.15). Host-tested and compiled; the owner's kit ran it on 17 October 2026.

- Tooling: `clang-format` 23.1.3 and `ruff` 0.17.0, with the same pins in `requirements.lock` ([#139](https://github.com/adidshaft/grok-gadgets-esp32-sdk/issues/139)). Sources need no reformatting; firmware and library are unchanged.

## 0.2.0-alpha.15 — 14 October 2026

- On 14 October 2026 the owner's kit, on this firmware and factory-provisioned once, joined home Wi-Fi from an iPhone and from an Android phone through its setup hotspot, with no computer and no app, and went Online through the relay. A wrong setup code was refused, a wrong Wi-Fi password showed on the page and the OLED, the 10-minute window closed, and two BOOT resets re-joined. A board provisioned before this change came back Online after the update ([hub record](https://github.com/adidshaft/grok-gadgets/blob/dev/docs/verification/phone-setup-2026-10-14.md)).

- Setup hotspot with a captive page ([#114](https://github.com/adidshaft/grok-gadgets-esp32-sdk/issues/114), hub ADR 010). `GrokSetup.h`: a board with a factory identity and no home Wi-Fi opens `GrokGadget-XXXX` with the label's WPA2 passphrase, for one phone, for 10 minutes. The page at `192.168.4.1` (captive DNS; every other path redirects there) asks for the label's setup code, lists 2.4 GHz networks, joins the chosen one and shows live status: joining, the join failure reason, joined, Online. Wrong codes: 5 free, then a doubling 1-minute lock; 20 close the hotspot. The page never shows a stored value. After a join the board saves only the home Wi-Fi, connects to the relay with the factory identity, and closes the hotspot 30 s after Online. The OLED kit shows `Setup N min` and the hotspot name, then `Joining WiFi` or the reason. `WifiLink::begin` can keep the hotspot up. Wi-Fi build: static RAM 108,076 B and flash 932,549 B (+8.5 KB RAM and +45.4 KB flash against 0.2.0-alpha.14). Host-tested and compiled; the owner's kit ran it on 14 October 2026.

- Factory identity separate from user settings ([#115](https://github.com/adidshaft/grok-gadgets-esp32-sdk/issues/115), hub ADR 010). NVS namespace `grok-factory` holds the device token, relay name and port, CA, setup-hotspot passphrase and setup code. The console command `factory_provision` writes it once (`replace` to change it) and never echoes a value. `grok-wifi` holds the home Wi-Fi. A BOOT reset or `factory_reset` erases `grok-wifi` only. A full `provision` line and boards provisioned before this change keep working. `status` reports `factory`. Static RAM +6.9 KB and flash +2.6 KB on the Wi-Fi build. Host-tested and compiled; the owner's kit ran it on 14 October 2026.

## 0.2.0-alpha.14 — 14 October 2026

- On 13 October 2026 the owner's kit, on this firmware, reached a gateway relay on an always-on Linux server by DNS name. Grok Bot on the iPhone, on cellular, showed text on it while the Mac slept. A wrong CA showed `TLS rejected`; revoke showed `Token rejected` ([hub record](https://github.com/adidshaft/grok-gadgets/blob/dev/docs/verification/mac-free-route-2026-10-13.md)).
- A relay's DNS name for the Wi-Fi gateway ([#111](https://github.com/adidshaft/grok-gadgets-esp32-sdk/issues/111), hub ADR 009). `provision` accepts a public DNS name such as `devices.example.net` (lower case, two or more labels, up to 63 characters) beside an IPv4 address and a `.local` name. `WifiLink` looks the name up before each connection and checks the certificate against it. The OLED kit's top row shows `Relay...` while it connects and `Relay name?` if the name does not resolve. Host-tested and compiled; the owner's kit ran it on 14 October 2026.
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
