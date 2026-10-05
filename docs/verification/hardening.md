# HARD-ESP-001 verification — 4 October 2026

Local correction for retained command acknowledgements. Electrical board simulation, host transport and real target compilation remain separate evidence levels. Physical C124, actual Grok, mobile, Linux USB permissions and independent human reproduction are pending.

## Source and environment

Started from clean SDK `41ee9635efeef553197e3d15363e02146d87e773`, except the coordinator-created HARD-ESP-001 local issue. Branch `fix/ack-retry-hard-esp-001`; exclusive firmware subagent ownership. Delegated to a bounded agent with its own context, confirmed by the coordinator. Initial checks ran against this base plus the correction's uncommitted source; final checks ran against clean integrated main `bca5d4e75123034a973be6b3837d75ac2c02a2c2`.

macOS 27.0 arm64; AppleClang 21.0.0 C++14; PlatformIO Core 6.1.18; espressif32 6.10.0; Arduino-ESP32 package 3.20017.0 (framework 2.0.17); Xtensa ESP32-S3 and RISC-V 8.4.0+2021r2-patch5; esptool 1.40501.0 (4.5.1); SCons 4.40801.0 (4.8.1); ArduinoJson 6.21.5; NeoPixel 1.12.3. ArduinoJson headers are the actual installed `.pio/libdeps/atoms3-lite-usb/ArduinoJson/src`, shared by host checks and board compilation.

## Failure and correction

During the 4 October 2026 correction session (first build checkpoint 16:19 UTC), an isolated `counter.bump` probe ran an original command followed by four identical retries against the installed headers. Calls 1/2 returned `{"type":"ack","command_id":"retry-probe","status":"executed","state":{"counter":1}}`; calls 3/4/5 returned JSON `true`. Execution count remained one. Before/after raw probe output remains under ignored `build/hardening/retry-before.log` and `retry-after.log`.

Mutable ArduinoJson input selected zero-copy parsing and modified the retained buffer. `Device::execute` now explicitly passes a `const char*`, preserving its cached serialization and owning replay strings. Deserialization failure returns a bounded `failed` ACK with `ack_unavailable` and empty state, without running the handler or replacing the retained result. That failure means the execution result is unavailable; it does not imply the original action failed. A correctly sized destination can retrieve the original result later.

The repeatable regression is in `tests/device_tests.cpp`: four retries each of interleaved success/failure IDs produce identical original state/error snapshots and one handler attempt each. Changed arguments conflict without replacing or extending the entry. A 512-byte destination deliberately cannot decode a retained 1000-character state: four explicit replay failures execute nothing, then a 4096-byte destination recovers the original ACK. Clearing the input after a replay leaves the result valid. Eight distinct IDs are retained FIFO; boundary, ninth-ID eviction and a new Device instance model reboot loss.

`tests/firmware_tests.cpp` compiles the actual `src/main.cpp` with simulated board/serial APIs. It repeats successful and failed poll commands, interleaves IDs, checks changed-argument conflict and unchanged pixel `show()` count. Both new library and actual-consumer regressions were compiled against the original header (copied using `git show 41ee963:lib/GrokGadgets/src/GrokGadgets.h`): the library equality assertion and firmware ACK-type assertion failed, exit 134. This confirms the regressions expose the original defect.

One initial regression helper was named `serialized`; ArduinoJson argument-dependent lookup selected its similarly named template and caused an ambiguous equality compile error. Renaming it to `jsonText` resolved the test-build failure. One toolchain probe tried `pio pkg list --json-output`, which this pinned PlatformIO does not support; packaging uses the supported plain resolved-package listing instead. Packaging deliberately rejects a dirty checkout, exit 1, before writing provenance.

## Commands and observed results

All commands run from the SDK unless the hub is stated. They passed before the implementation commit and again on clean integrated main `bca5d4e75123034a973be6b3837d75ac2c02a2c2`:

| Command | Result |
| --- | --- |
| `sh tools/check.sh` | exit 0; Ruff and clang-format pass, host CTest 3/3 including new retries |
| `.venv/bin/python tools/check_contract.py` | exit 0; pinned fixtures and generated frames validate, <=2048 bytes including LF |
| `../grok-gadgets-gateway/.venv/bin/python tools/check_gateway.py` | exit 0; actual firmware host -> PTY -> USB bridge -> authenticated gateway, four redeliveries accepted as identical ACKs in the same session; previous green/off, 20-edge overflow/loss, restart and revocation/restoration pass |
| `.venv/bin/pio run -e atoms3-lite-usb` | exit 0 SUCCESS; RAM 54,628/327,680; program flash 274,881/3,342,336 |
| Hub `python3 scripts/check.py` | exit 0; 19 labeled issues and syntax verified |

The USB test's local `RetryGateway` fault injection redelivers the same completed command through ordinary poll responses; production gateway command semantics stay unchanged. It records ACKs only after canonical validation and duplicate acceptance. Single handler execution is asserted at the library and actual consumer `show()` boundary. The PTY integration passed with clean gateway `ac12b442674b9dcf010efa9d6d0a5cf54fe1cdf2`; subsequent coordinated checks use clean final gateway `aeabcaf46cca830894836ac5cb85f3a6d33cd63d`. Canonical schemas/fixtures remain byte-identical; the SDK now pins that exact commit and includes the authoritative README hash `b1ae2a5c8ea0933eb9bcedce01e8c7bbed5215591229a78932637f2a76f07ac1`, covering clarified command/event roles and bounded per-device/current-boot event windows.

## Package checkpoint (historical)

Implementation commit: `851ffab2a9fa81feb20f9f2b3c2e044b78224b33`. Clean-source packaging ran at 16:22:29 UTC with that commit; host, contract and PTY checks also passed on its clean source. Final main package after canonical pin refresh: clean integrated source `bca5d4e75123034a973be6b3837d75ac2c02a2c2`, rebuilt at `2026-10-04T16:24:01.474053+00:00`, RAM 54,628 and program flash 274,881 bytes. The table below is that snapshot. It is not the current image. The only current checksum record is [build checksums](../build-checksums.json). `firmware.bin` depends on the Xtensa toolchain package `system` as well as the version pin. `-ffile-prefix-map` removes checkout and PlatformIO home paths from later images.

| Output | Bytes | SHA-256 |
| --- | --- | --- |
| firmware.bin | 283424 | `de30fff189e37682ede9613bfccabb39550b5fa30d606eab797da4c49b80de25` |
| firmware.elf | 7556144 | `cfa5baa2e6709cb6581b1add5a09142531c56db1ad82e09c5b7cfddc90c933f2` |
| bootloader.bin | 15104 | `1776e4dd896a69d0a5c2e79957b0e2a88aa4129b1381d6478683515a1f6af343` |
| partitions.bin | 3072 | `1d9cca96de0fe07ad7fc0648b9878ddecd9ce565e38b589ad20fea698ed4c80c` |


## Open external gates

ESP-004 requires physical C124 and flashing authorization. ESP-006 requires another human's reproduction. ESP-005 needs an agreed secure reachable Wi-Fi/provisioning contract. Actual Grok and mobile acceptance require account/transport authorization. No flashing, actual account access, public push/deploy, spending or automation occurred.
