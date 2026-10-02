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

Include SDK/gateway commits, OS/compiler/tool versions, exact command, expected and observed result, and whether the device was simulated or physical. Share only redacted logs. [Issues](https://github.com/adidshaft/grok-gadgets-esp32-sdk/issues/new/choose) are the planned public defect tracker; [local issues](planning/issues.json) remain authoritative until approved migration. General discussion goes to [r/GrokGadgets](https://www.reddit.com/r/GrokGadgets/). Private reports use [Security](SECURITY.md), and conduct reports use [adidshaft@kyokasuigetsu.xyz](mailto:adidshaft@kyokasuigetsu.xyz).

Windows/Intel Mac installation, Linux USB permissions, physical C124 behavior, Wi-Fi, mobile and actual Grok invocation are open gates. A software reproduction can help diagnose them, but cannot close their physical/account requirements.
