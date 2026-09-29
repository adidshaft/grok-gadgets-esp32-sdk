# Local verification evidence — 4 October 2026

Environment: macOS 27.0, darwin_arm64; Python 3.14.7; AppleClang 21.0.0; CMake; PlatformIO Core 6.1.18. This is Mac host and ESP32-S3 cross-compiler evidence, not Linux runtime validation.

| Check | Observed result |
| --- | --- |
| `sh tools/check.sh` | Python lint/format and C++ format pass; CTest 3/3 pass (portable core, capability device, actual firmware consumer host simulation) |
| `.venv/bin/python tools/check_contract.py` | Pinned canonical fixture and SDK-generated hello/ACK/event/poll/state validate; every generated frame <=2048 bytes including LF |
| `../grok-gadgets-gateway/.venv/bin/python tools/check_gateway.py` | Actual firmware main.cpp compiled against simulated board/serial -> PTY -> USB bridge -> authenticated DeviceServer passes green/off state/execution, server socket restart, fresh session, revoked credential disconnect and restoration |
| `.venv/bin/pio run -e atoms3-lite-usb` | Real ESP32-S3 firmware compile SUCCESS; RAM 54,628 /327,680; program flash 276,097 /3,342,336 |

Host firmware harness identifies `simulated:true`; actual compiled C124 USB firmware identifies `simulated:false`. The electrical board is simulated in host tests. These are complementary evidence levels: **simulated** and **build verified**. No physical LED, button, USB enumeration, flash, Wi-Fi, real Grok or mobile interoperability is claimed.

Gateway integration uses clean main `4cf42fffa32afa2e5ad022e3fa4797f474ba10a9`; canonical schema source is `17d31686ad06d608f20b117e6f4070bacbff7a35` plus SHA-256 manifest. SDK core first integrated as `ff4dd9d2e6766da3ee9a5d03fef8332d3cff49e2`. Compile/host logs are preserved in ignored build/; [build-checksums.json](build-checksums.json) records actual binary hashes. Local artifacts are generated into ignored artifacts/c124-usb with a source-commit manifest. Python 3.14 produced one warning inside esptool's third-party Python source during its first build; compilation succeeded. It does not indicate that a physical device was reached.

A fresh Python virtual environment using requirements.lock is exercised during local packaging; shared downloaded PlatformIO package cache is reused. This checks installation, not an independent person's reproduction or an empty-cache toolchain download on Linux. Prepared GitHub Linux workflows have not run or been activated.

Open gates: physical C124/flashing authorization; real Grok account and authenticated reachable route; Linux serial permissions/USB acceptance; independent tester; publication permission; secure Wi-Fi endpoint/provisioning contract. See [issues](../planning/issues.json) and [physical procedure](build-flash.md).
