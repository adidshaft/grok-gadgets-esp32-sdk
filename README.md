# Grok Gadgets ESP32 SDK

This library builds an ESP32 gadget that speaks the Grok Gadgets device protocol over USB to a gateway on the same computer. The first sketch is an M5Stack AtomS3 Lite (SKU C124): one RGB LED and a button. [examples/led-button](examples/led-button/main.cpp) is a generic ESP32-S3 LED and button. The firmware is Arduino, not ESPHome. It is an experimental alpha. Original code is Apache-2.0.

## What works with Grok Bot today

The C124 sketch compiles for ESP32-S3. It has not been flashed, and no LED, button, or USB behavior was observed on hardware here. Reaching Grok Bot would need this gateway on the same computer and a separate operator tunnel. That path was not run. There is no hosted device service in this repository, and no Grok check was completed.

## Quickstart

1. Install Python 3.11 or later, CMake 3.16 or later, Git, and a C++14 compiler.
2. Run the host checks and compile the C124 firmware. Success means **build verified, hardware pending**.
3. Read [Build, flash and recovery](docs/build-flash.md) before connecting a board. Upload with PlatformIO so the bootloader, partitions, `boot_app0.bin`, and app land at their offsets.
4. For a board, set `GROK_GADGETS_DEVICE_TOKEN` for the USB bridge. `grok-gadgets-gateway enroll` and `grok-gadgets-gateway serve` are the intended host commands. They were not run here.

```sh
python3 -m venv .venv
.venv/bin/pip install -r requirements.lock
.venv/bin/pio pkg install
sh tools/check.sh
.venv/bin/python tools/check_contract.py
.venv/bin/pio run -e atoms3-lite-usb
```

Declare `grok::Gadget` as a global or static object. It is about 40 KB and does not fit the 8 KB `loop()` stack.

## Details

| Next step | Guide |
| --- | --- |
| Flash offsets, device ID, token, and unplug recovery | [Build, flash and recovery](docs/build-flash.md) |
| Commands, events, and limits | [Library](docs/sdk.md) |
| Retries, loss reports, and reconnects | [Protocol and recovery](docs/protocol-recovery.md) |
| C124 pins, and the GPIO48 `RGB_BUILTIN` trap | [Board sources](docs/board-sources.md) |
| What was checked, and what is still open | [Verification](docs/verification.md) |
| The one firmware checksum record | [build-checksums.json](docs/build-checksums.json) |

Host tests compile the sketches against a simulated board. They do not open a serial port. The contract check validates frames those sketches write, plus the pinned protocol fixture. `firmware.bin` hashes also depend on the Xtensa toolchain host variant; see the checksum record.

The gateway device port is authenticated loopback on the same computer. Cloud access needs a separate authenticated HTTPS MCP route (`HARD-GROK-REMOTE-001`). See the [hosting FAQ](https://github.com/adidshaft/grok-gadgets/blob/main/docs/getting-started/hosting.md).

For build errors see [Support](SUPPORT.md). Report defects through the [issue chooser](https://github.com/adidshaft/grok-gadgets-esp32-sdk/issues/new/choose). Vulnerabilities go through [Security](SECURITY.md). [Contributing](CONTRIBUTING.md) covers software, docs, and hardware evidence. [Dependency licenses](docs/dependencies.md) and [NOTICE](NOTICE) apply before any firmware redistribution. This project is unaffiliated with xAI and M5Stack.

Pre-publication commit dates were reconstructed across 29 September–5 October 2026. Verification records keep their execution dates. See the [history and privacy record](https://github.com/adidshaft/grok-gadgets/blob/main/docs/verification/publication-sanitization.md).
