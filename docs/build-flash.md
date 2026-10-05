# Build, flash and recover C124

This example uses the reusable library in `lib/GrokGadgets`. It is standalone Arduino firmware, not an ESPHome integration.

Use **M5Stack AtomS3 Lite SKU C124**, with ESP32-S3FN8 and 8 MB flash. ATOM Lite and AtomS3 with a display are different boards.

## Clean setup

You need Python 3.11 or later, CMake 3.16 or later, Git and a C++14 compiler. The recorded local build used Python 3.14.7 on macOS. Linux USB operation remains unverified.

From the repository root, run:

```sh
python3 -m venv .venv
.venv/bin/pip install -r requirements.lock
.venv/bin/pio pkg install
sh tools/check.sh
.venv/bin/python tools/check_contract.py
.venv/bin/pio run -e atoms3-lite-usb
```

The pinned tools and libraries are:

| Dependency | Version |
| --- | --- |
| PlatformIO | 6.1.18 |
| espressif32 | 6.10.0 |
| Arduino-ESP32 | 2.0.17 |
| Xtensa compiler | 8.4.0+2021r2-patch5 |
| ArduinoJson | 6.21.5 |
| NeoPixel | 1.12.3 |

Verification records contain the full resolved package list and hashes. Downloads need internet access and local disk space. Mac host tests do not verify Linux USB permissions or physical pins.

## Inspect build output

The build writes these files in `.pio/build/atoms3-lite-usb/`:

- `firmware.bin`
- `firmware.elf`
- `bootloader.bin`
- `partitions.bin`

Git ignores build output. `tools/package_build.py` copies these files into `artifacts/c124-usb` and writes checksums. A successful compile means **build verified, hardware pending**.

To create a package with source records:

1. Commit the tested source.
2. From a clean checkout, run `.venv/bin/python tools/package_build.py`.
3. Update `docs/build-checksums.json` from the resulting manifest.
4. Record that update in a separate evidence commit.

Packaging rebuilds the exact commit. It records clean source state, UTC build time, toolchain versions and SHA-256 hashes. It rejects uncommitted source changes.

## Flash when hardware is available

These hardware steps remain unverified. They are separate from the software quickstart. Select and authorize the intended physical board first.

1. Connect C124 with a USB-C **data** cable.
2. Run `.venv/bin/pio device list`.
3. Identify the board's port. Do not guess another device's port.
4. Close any serial monitor or bridge.
5. Replace the port placeholder and upload:

```sh
.venv/bin/pio run -e atoms3-lite-usb -t upload --upload-port /dev/cu.usbmodemYOUR_BOARD
```

On Linux, the port is often `/dev/ttyACM0`. Inspect the actual port before use. Linux can require membership in a serial-device group, often `dialout`. You can need to reconnect or log in again after that change. Do not run the entire gateway as root.

The example sends no debug logs on the protocol CDC channel.

## Connect the USB bridge

The device ID is `c124-<MAC hex>`. Each boot creates a random boot ID. Credentials remain on the host.

1. Open a serial monitor at 115200 briefly to inspect hello.
2. Record the device ID for the gateway credential.
3. Close the monitor.
4. Set `GROK_GADGETS_DEVICE_TOKEN` in your private shell environment.
5. Follow the [gateway USB bridge guide](https://github.com/adidshaft/grok-gadgets-gateway/blob/main/docs/local-operation.md#usb-bridge).

The bridge command is `.venv/bin/python -m grok_gadgets_gateway.usb_bridge PORT`. The bridge supplies the token to the loopback connection. Only one process can use the serial connection at a time.

## Recover the board

If the board does not appear as a USB device, follow [M5Stack's C124 download-mode procedure](https://docs.m5stack.com/en/core/AtomS3-Lite):

1. Hold the reset button for about two seconds, until the green indicator appears.
2. Release the button.
3. Identify the new port.
4. Upload to that port.

The reset/boot control is different from the front user button. If the board still does not appear, use a direct USB port and a known data cable.

For an unreliable upload connection, reduce upload speed in `platformio.ini`. Record this build change.

## Restore a previous version

1. Open a clean checkout of a previously tested commit.
2. Repeat that commit's pinned build.
3. Upload with `pio run -t upload`.

This command supplies the bootloader, partition and firmware offsets. Do not write only `firmware.bin` to a guessed address.

For factory restoration, use the manufacturer's M5Burner or Easyloader route. Preserve its source and license. Verify the exact C124 target. No firmware was flashed during the recorded local phase.

## First physical acceptance

Record SDK and gateway commits, firmware checksums, operating system, port, timestamps and observations.

1. Discover C124.
2. Request green, another colour and off. Observe each result.
3. Press and release the user button. Check the order of reported events.
4. Unplug the board during a command. Check offline and unconfirmed status.
5. Reconnect and check fresh state.
6. Reboot the board and gateway. Check boot and cursor resets.
7. Repeat through Grok only after its account and transport route is verified.

A compiled binary or firmware acknowledgement cannot replace these observations.
