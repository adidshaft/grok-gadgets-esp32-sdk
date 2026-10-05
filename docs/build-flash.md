# Build, flash and recover C124

This example uses the reusable library in `lib/GrokGadgets`. It is standalone Arduino firmware, not an ESPHome integration.

Use **M5Stack AtomS3 Lite SKU C124**, with ESP32-S3FN8 and 8 MB flash. ATOM Lite and AtomS3 with a display are different boards.

Builds and local software tests need no public hosting. For the USB route, run the bridge
and gateway on the same computer. That computer must remain on during use.
The gateway offers local stdio or authenticated HTTP MCP through `serve` at
`http://127.0.0.1:8766/mcp`. Its separate device port is authenticated loopback TCP.
Public HTTPS, OAuth, and actual Grok Bot use remain separate gates. Do not expose the device port through a tunnel.
See the [hosting FAQ](https://github.com/adidshaft/grok-gadgets/blob/main/docs/getting-started/hosting.md)
for who operates each process and the `HARD-GROK-REMOTE-001` remote-access gate.

## Clean setup

You need Python 3.11 or later, CMake 3.16 or later, Git and a C++14 compiler. The recorded local build used Python 3.14.7 on macOS. Linux USB operation remains unverified.

Run the [README quickstart](../README.md#quickstart) from the repository root. It installs the pinned tools, runs host checks, and compiles C124 without hardware.

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

Git ignores build output. `tools/package_build.py` copies these files, plus `boot_app0.bin` and `flash_args`, into `artifacts/c124-usb` and writes checksums. A successful compile means **build verified, hardware pending**.

[docs/build-checksums.json](build-checksums.json) is the only current checksum record. `firmware.bin` and `firmware.elf` follow the installed Xtensa package `system` (`darwin_arm64`, `darwin_x86_64`, or a Linux value), not only the version string `8.4.0+2021r2-patch5`. The same source built with another host variant is a different image. Builds pass `-ffile-prefix-map` so the checkout path and the PlatformIO home are outside the image hash. Bootloader and partition hashes omit those paths. Hash tables in the verification notes are historical snapshots of older commits.

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

`pio run -t upload` is the flash path for this environment. It writes:

| File | Offset |
| --- | --- |
| `bootloader.bin` | `0x0` |
| `partitions.bin` | `0x8000` |
| `boot_app0.bin` | `0xe000` |
| `firmware.bin` | `0x10000` |

PlatformIO uploads this Arduino board as DIO, 80 MHz, 8 MB. It rewrites the board file's QIO mode to DIO. No board was flashed in this repository. `flash_args` in the local package records the same offsets for inspection.

## Connect the USB bridge

The device ID is `c124-` plus the MAC in the order esptool prints, lowercase hex, no separators. MAC `bc:9a:78:56:34:12` is `c124-bc9a78563412`. Each boot creates a new random boot ID. Credentials stay on the host. The USB hello has no credential.

The bridge reads `GROK_GADGETS_DEVICE_TOKEN` from the environment and adds that token on the loopback connection. Keep the token out of Git.

Install the [gateway](https://github.com/adidshaft/grok-gadgets-gateway) on this computer, then follow its [enrollment and USB bridge steps](https://github.com/adidshaft/grok-gadgets-gateway/blob/main/docs/local-operation.md#per-device-credential-enrollment):

1. Run `grok-gadgets-gateway init` once to create the private MCP token and device registry.
2. Enroll the exact device ID above with `grok-gadgets-gateway enroll DEVICE_ID`.
3. Keep `grok-gadgets-gateway serve` running in one terminal.
4. In another terminal, set `GROK_GADGETS_DEVICE_TOKEN` to the value printed once by enrollment. Start `python -m grok_gadgets_gateway.usb_bridge PORT` with the identified serial port.

Use the gateway's Python environment for these commands. The device token protects the local connection; it is separate from the MCP bearer token. The commands exist on `simplify-and-fix`, but no physical C124 or Grok Bot session has used them here.

If USB disconnects, the bridge retries the same serial path. Plugging the board back in power-cycles it, so the boot ID changes. If the operating system assigns a different port, stop the bridge, identify that port, and restart with it. Automatic recovery is software-tested; physical USB recovery remains unverified.

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
2. Repeat that commit's pinned build with the same Xtensa toolchain package `system`.
3. Upload with `pio run -t upload`.

That command writes the four images at the offsets above.

For factory restoration, use the manufacturer's M5Burner or Easyloader route. Preserve its source and license. Verify the exact C124 target. No firmware was flashed during the recorded local phase.

## First physical acceptance

Record SDK and gateway commits, firmware checksums, operating system, port, timestamps and observations.

1. Discover C124.
2. Request green, another colour and off. Observe each result.
3. Press and release the user button. Check the order of reported events.
4. Unplug the board during a command. Check that the gateway marks it offline after its timeout and does not report an unobserved result as confirmed. The bridge should keep retrying.
5. Plug the board back in. USB power starts a new boot, so the boot ID changes. Check automatic recovery and fresh state. Restart the bridge only if the port name changed.
6. Reboot the board and gateway. Check boot and cursor resets.
7. Repeat through Grok Bot only after its account and transport route is verified.

A compiled binary or firmware acknowledgement cannot replace these observations.
