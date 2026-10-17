# ESP-CORE-001: Arduino-ESP32 2.0.17 or 3.3.x for Phase 12b

**Date:** 17 October 2026 · **Issue:** [#124](https://github.com/adidshaft/grok-gadgets-esp32-sdk/issues/124) · **Status:** recommendation for the hub ADR 012 review

The SDK pins Arduino-ESP32 2.0.17 (ESP-IDF 4.4.7). Phase 12b adds signed over-the-air
updates. This page records whether the core must move to 3.3.x first. No code moved: the
3.3.x numbers come from a scratch copy of this repository with only the platform line
changed.

## Build matrix

Both builds from ESP32 `dev` at `986a876` (0.2.0-alpha.16 content), on macOS
(Intel), with PlatformIO Core 6.2.0. "RAM" is static data and BSS as PlatformIO reports it;
"Flash" is the application image against one 6,553,600-byte app slot of
`default_16MB.csv`.

| Build | Core | Platform | RAM (bytes) | Flash (bytes) | Result |
| --- | --- | --- | ---: | ---: | --- |
| `esp32s3-oled-kit` (USB) | 2.0.17 (IDF 4.4.7) | `espressif32@6.10.0` | 59,656 | 305,641 | Builds |
| `esp32s3-oled-kit` (USB) | 3.3.12 (IDF 5.5.5) | pioarduino `55.03.312-1` | 71,688 | 448,612 | Builds |
| `esp32s3-oled-kit-wifi` | 2.0.17 (IDF 4.4.7) | `espressif32@6.10.0` | 108,108 | 933,361 | Builds |
| `esp32s3-oled-kit-wifi` | 3.3.12 (IDF 5.5.5) | pioarduino `55.03.312-1` | 110,988 | 1,096,743 | Builds, 2 warnings |

On 3.3.12 the Wi-Fi image grows by 163,382 bytes (17.5 %) and static RAM by 2,880 bytes.
Both fit the slot with room for an update.

## API deltas found

The sources compiled on 3.3.12 without a change. Compiling is not a behaviour test; nothing
ran on the kit.

| Area | 2.0.17 → 3.3.12 |
| --- | --- |
| `WiFiClientSecure` | Compiles unchanged (`setCACert`, `connect`, timeouts) |
| mDNS (`ESPmDNS`) | Compiles unchanged |
| NVS (`Preferences`) | Compiles unchanged |
| `WiFi` events | Compiles unchanged; the `ARDUINO_EVENT_*` names already exist in 2.0.17 |
| `Update` | Compiles unchanged; still MD5 only, no signature check |
| `WebServer`, `DNSServer` | Compiles unchanged |
| RGB pixel | `neopixelWrite` is deprecated: "Use rgbLedWrite() instead" (the 2 warnings) |
| mbedTLS | 2.28.7 → 3.6.6. The public-key verify calls (`mbedtls_pk_parse_public_key`, `mbedtls_pk_verify`) exist in both; struct fields became private in 3.x |
| Provisioning | Security 0 and 1 → 0, 1 and 2 (SRP6a) |

## What Phase 12b needs, on each core

From each core's prebuilt `sdkconfig.h` for the kit's `qio_opi` memory type:

| Need | 2.0.17 | 3.3.12 |
| --- | --- | --- |
| Two app slots and `otadata` | Yes (`default_16MB.csv`: `app0`, `app1`, 6.25 MiB each) | Same table |
| App rollback (`CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE`) | On | On |
| Defer "valid" until the app decides (`verifyRollbackLater()`) | Weak hook in `initArduino()` | Same hook |
| ECDSA P-256 (`CONFIG_MBEDTLS_ECDSA_C`) | On | On |
| Signed apps without secure boot (`CONFIG_SECURE_SIGNED_APPS_NO_SECURE_BOOT`) | Off | Off |
| Anti-rollback (`CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK`) | Off | Off |

Neither prebuilt core verifies a signature in the bootloader or in `Update`. On both, the
signature check has to be in the application, over the streamed image, before
`Update.end()`. The 3.x core adds nothing that Phase 12b needs.

## Risks of moving now

- A core move changes the bootloader that USB flashing writes. An over-the-air update
  replaces the app only, so a kit flashed with the 2.0.17 bootloader would run a 3.x app on
  an IDF 4.4 bootloader. We did not test that pairing.
- The Wi-Fi, TLS, setup hotspot and claim acceptance (Phases 4–11) all ran on 2.0.17. A move
  repeats them.
- +163 KB per image means about 160 more chunks per update over the relay.

## Recommendation

**No-go for Phase 12b.** Keep 2.0.17. Build Phase 12b's signature check on the mbedTLS
public-key API, which exists in both cores, so the code moves with the core later.

**Go later, as its own step:** migrate in a later phase, once over-the-air updates work. First send a 3.x image over the air to a spare board with a 2.0.17 bootloader. Only then flash a kit with a 3.x bootloader. Rename `neopixelWrite` to `rgbLedWrite` then.

The owner decides at the ADR 012 review.
