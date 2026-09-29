# Grok Gadgets ESP32 SDK

A reusable C++ capability SDK for ESP32 gadgets that connect to the Grok Gadgets gateway. The first consumer is standalone Arduino firmware for **M5Stack AtomS3 Lite C124**, with built-in RGB LED/button and USB communication.

**Local alpha: build verified, hardware and Grok pending.** Firmware compiled for ESP32-S3; host tests exercised the actual firmware loop with simulated GPIO/serial and authenticated gateway integration over a pseudo-terminal USB connection. No board was flashed, no physical LED/button observed, and no real Grok Bot connected. Wi-Fi awaits a secure reachable transport/provisioning contract; protocol 0.1.0 currently binds gateway loopback only.

## Build and test

```sh
python3 -m venv .venv
.venv/bin/pip install -r requirements.lock
.venv/bin/pio pkg install
sh tools/check.sh
.venv/bin/python tools/check_contract.py
.venv/bin/pio run -e atoms3-lite-usb
```

Output: `.pio/build/atoms3-lite-usb/firmware.bin`. Run `.venv/bin/python tools/package_build.py` for unpublished binaries/checksums in `artifacts/c124-usb`. See [build/flashing/recovery](docs/build-flash.md), [SDK use and custom capability example](docs/sdk.md), [framing and recovery semantics](docs/protocol-recovery.md), [manufacturer pin sources](docs/board-sources.md), and [verification evidence](docs/verification.md).

The library supports custom command handlers, reported state, strict RGB arguments and bounded duplicate protection. ArduinoJson 6.21.5 is pinned; portable framing/debounce/queue utilities need only C++14. [Protocol schemas](protocol/0.1.0/device-request.schema.json) are pinned from the canonical gateway repository with [source checksums](protocol/source.json). Clone this repository alone to build and use the SDK; sibling gateway is needed only for connection/integration tests.

See [contribution instructions](CONTRIBUTING.md), [local issues](planning/issues.json), and shared [community policies](../grok-gadgets/community/README.md). Independent project unaffiliated with xAI or M5Stack. Original code is Apache-2.0; preserve [dependency licenses](docs/dependencies.md) for firmware redistribution. Public repository, release, activated CI and physical support remain pending.
