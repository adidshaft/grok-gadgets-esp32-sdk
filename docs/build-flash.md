# Build, flash, and recover C124

This is standalone Arduino firmware consuming the reusable library in `lib/GrokGadgets`, not an ESPHome recipe. Target **M5Stack AtomS3 Lite SKU C124**, ESP32-S3FN8, 8MB flash. ATOM Lite and display AtomS3 are different boards.

## Clean setup

From this repository on macOS or Linux with Python 3.11+ (the local build used 3.14.7), CMake 3.16+, Git and a C++14 compiler:

```sh
python3 -m venv .venv
.venv/bin/pip install -r requirements.lock
.venv/bin/pio pkg install
sh tools/check.sh
.venv/bin/python tools/check_contract.py
.venv/bin/pio run -e atoms3-lite-usb
```

PlatformIO 6.1.18, espressif32 6.10.0, Arduino-ESP32 2.0.17, Xtensa compiler 8.4.0+2021r2-patch5, ArduinoJson 6.21.5 and NeoPixel 1.12.3 are pinned. The complete resolved package list and hashes are recorded in verification evidence. Tool downloads require internet and local disk space. Host logic tests on a Mac do not validate Linux USB permissions or physical pins.

Outputs: `.pio/build/atoms3-lite-usb/firmware.bin`, `firmware.elf`, `bootloader.bin`, `partitions.bin`. Build outputs are ignored by Git; `tools/package_build.py` copies them into `artifacts/c124-usb` and writes checksums. These are **build verified, hardware pending** only after the recorded compile succeeds.

For a provenance package, commit the tested source first, then run `.venv/bin/python tools/package_build.py` from a clean checkout. Packaging rebuilds that exact commit and records its clean source state, UTC build time, resolved toolchain and SHA-256 hashes. It rejects uncommitted sources rather than labeling an older binary as the current commit. Refresh the tracked `docs/build-checksums.json` from that manifest in a subsequent evidence commit.

## Flash when hardware is available

These are prepared hardware steps, not steps used during the software quickstart. Select and authorize the intended physical board before proceeding.

Use a USB-C **data** cable. Inspect ports with `.venv/bin/pio device list`, identify this board rather than guessing another device's port, close any serial monitor/bridge, then:

```sh
.venv/bin/pio run -e atoms3-lite-usb -t upload --upload-port /dev/cu.usbmodemYOUR_BOARD
```

Linux port is commonly `/dev/ttyACM0`, but inspect it. Linux may require membership in the OS's serial-device group (often dialout) and reconnect/login afterward; never run the entire gateway as root. The normal example has no log prints on the protocol CDC channel.

The device ID is `c124-<MAC hex>` and boot ID is random per boot. To inspect hello before registering a gateway credential, run the serial monitor at 115200 briefly. It prints no token because credentials stay on the host. Close monitor before starting the gateway USB bridge. See the [gateway USB bridge guide](https://github.com/adidshaft/grok-gadgets-gateway/blob/main/docs/local-operation.md#usb-bridge) for `.venv/bin/python -m grok_gadgets_gateway.usb_bridge PORT` (set `GROK_GADGETS_DEVICE_TOKEN` in your private shell environment); gateway supplies the token to the loopback connection. USB owns one serial connection at a time.

## Recovery / rollback

If the board stops enumerating, follow [M5Stack's C124 download-mode procedure](https://docs.m5stack.com/en/core/AtomS3-Lite): hold the reset button about two seconds until the green indicator appears, then release and identify the newly enumerated port before uploading. Do not confuse this reset/boot control with the front user button. Use a direct USB port and known data cable if enumeration fails. Reduce upload speed in platformio.ini if the connection is unreliable and record that build change.

To roll back, check out a previous tested commit in a clean checkout, repeat its pinned build and upload. Prefer `pio run -t upload`, which supplies the correct bootloader/partition/firmware offsets, over writing only firmware.bin at an improvised address. Factory restoration remains the manufacturer M5Burner/Easyloader route; preserve its source/license and verify the exact C124 target. No firmware was flashed during this local phase.

## First physical acceptance

Record SDK/gateway commits, firmware checksums, OS/port, timestamps and observations. Discover C124; request green, another colour, then off and visibly inspect each. Press/release the real user button; read ordered edges. Unplug during a command and verify offline/unconfirmed; reconnect and verify fresh state. Reboot board and gateway to verify boot/cursor reset. Repeat through real Grok Bot only after its account/transport route is verified. A compiled binary or firmware ACK cannot replace those observations.
