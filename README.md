# Grok Gadgets ESP32 SDK

Build USB gadgets for your existing **Grok Bot**. Define commands and events in an Arduino sketch; `grok::Gadget` handles the connection to the local Grok Gadgets gateway. This is an experimental alpha. Original code is Apache-2.0.

## What works with Grok Bot today

**You can build and test the software now. Hardware and Grok Bot control remain unverified.** Two examples compile: the M5Stack AtomS3 Lite C124 RGB LED/button and a [generic ESP32-S3 LED/button](examples/led-button/main.cpp). Neither has been flashed here. Wi-Fi, hosted pairing, and button-triggered Bot tasks are not provided.

## Quickstart

You need Python 3.11+, CMake 3.16+, Git, and a C++14 compiler. From this repository, run:

```sh
python3 -m venv .venv
.venv/bin/pip install -r requirements.lock
.venv/bin/pio pkg install
sh tools/check.sh
.venv/bin/python tools/check_contract.py
.venv/bin/pio run -e atoms3-lite-usb
```

Expected: four host test suites pass, both sketch transcripts pass the contract check, and the C124 firmware compiles. No board or account is needed. This means **build verified, hardware pending**.

Next, [create your own gadget](docs/sdk.md), or [flash C124 and connect its USB bridge](docs/build-flash.md). The gateway and bridge run on the same computer. `enroll` issues a device token; `serve` keeps the local gateway running. Grok Bot needs a separately verified, authenticated remote route. A tunnel alone does not establish Bot compatibility.

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
