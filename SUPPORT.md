# Support

Start with the [standalone build](README.md#start-without-hardware), then identify the failed step.

| Symptom | First check | Expected outcome |
| --- | --- | --- |
| `.venv/bin/pio` or formatter is missing | Install `requirements.lock` into the repository's `.venv` | Pinned tools become available |
| CMake reports ArduinoJson headers missing | Run `.venv/bin/pio pkg install` before host checks | `.pio/libdeps/atoms3-lite-usb/ArduinoJson` exists |
| Pinned protocol checksum fails | Check local edits to `protocol/` and the recorded source manifest | Restore a coordinated tested pin; do not bypass the check |
| Firmware build cannot find packages | Check first-install internet access and local package cache | Pinned PlatformIO packages resolve |
| Board cannot be identified | Follow [board selection/recovery](docs/build-flash.md) with a known data cable | A verified C124 port is selected before upload |
| Gateway command is timed out/unconfirmed | Inspect reported state and the connection safely | Preserve uncertainty; do not retry a physical action under a fresh command ID |

## Report a defect

Use [GitHub Issues](https://github.com/adidshaft/grok-gadgets-esp32-sdk/issues/new/choose). Include:

- SDK and gateway commits.
- Operating system, compiler and tool versions.
- Exact command.
- Expected and observed results.
- Whether the device was simulated or physical.
- Logs with private data removed.

Use [r/GrokGadgets](https://www.reddit.com/r/GrokGadgets/) for general discussion. Use [Security](SECURITY.md) for private vulnerability reports. Send private conduct reports to [adidshaft@kyokasuigetsu.xyz](mailto:adidshaft@kyokasuigetsu.xyz).

## Verification limits

Windows and Intel Mac installation remain unverified. Linux USB permissions, physical C124 operation, Wi-Fi, mobile behavior and actual Grok invocation also remain unverified.

A software test can help diagnose these paths. It cannot replace physical observations or actual account tests.
