# Standalone build-guide rehearsal — 5 October 2026

The newcomer commands were run in a fresh temporary source directory without sibling repositories. Runtime source was committed `69716d8f1dc6c1b6853f2711fe611958ba42244a`; current README/build/policy working changes were copied into that archive. Runtime, host tests, protocol/schema pins, dependency locks, LICENSE and NOTICE were unchanged.

Environment: macOS 27.0 arm64, Python 3.14.7, AppleClang 21.0.0, C++14, pinned requirements and PlatformIO Core 6.1.18. Global downloaded PlatformIO tool packages/cache were reused; ArduinoJson and NeoPixel were installed into the fresh checkout. This verifies a new source directory and Python environment, not empty-cache Linux/Windows installation or independent-human reproduction.

| Standalone command | Observed result |
| --- | --- |
| `python3 -m venv .venv` using the selected Python 3.14 interpreter | Passed; fresh environment without system-site packages |
| `.venv/bin/pip install -r requirements.lock` | Passed; locked Python dependencies installed |
| `.venv/bin/pio pkg install` | Passed; ArduinoJson 6.21.5 and NeoPixel 1.12.3 installed |
| `sh tools/check.sh` | Passed; lint/format and CTest 3/3 |
| `.venv/bin/python tools/check_contract.py` | Passed; pinned canonical fixtures and generated frames valid |
| `.venv/bin/pio run -e atoms3-lite-usb` | SUCCESS; RAM 54,628/327,680; program flash 274,881/3,342,336 |

An initial optional offline uv-cache install failed because several pinned packages, including ajsonrpc, were absent from that cache. The guide does not promise offline installation. Re-running the documented pip installation in a fresh environment succeeded; the failed cache experiment remains in ignored local evidence.

The temporary compile produced a 283,424-byte firmware image plus ELF, bootloader and partition files. Their actual SHA-256 hashes and command results are in ignored `build/launch-docs/standalone-pip.json`; they identify that temporary working-source trial, not the current tree. `firmware.bin` follows the Xtensa toolchain package `system`, not the checkout path. The only current checksum record is [build checksums](../build-checksums.json).

Original code remains Apache-2.0; dependency license obligations are recorded in [dependencies](../dependencies.md) and root NOTICE. Source preparation does not authorize firmware binary redistribution. No board was flashed or observed, no USB port was opened, no Linux serial permissions were tested, and no Grok/home/mobile account was used. Physical C124, secure Wi-Fi and independent-human acceptance remain blocked under their existing issues.

## Committed build checkpoint (historical)

These hashes identify clean commit `304c518cfce8b7f5cb4a1604beae76f680af3460` on one host toolchain variant. They are not the current tree. The only current checksum record is [build-checksums.json](../build-checksums.json). `firmware.bin` also changes when the Xtensa package `system` changes (`darwin_arm64`, `darwin_x86_64`, or Linux) even when the version string matches.

The firmware image from that checkpoint is 283,424 bytes, SHA-256 `de30fff189e37682ede9613bfccabb39550b5fa30d606eab797da4c49b80de25`. The host environment was macOS 27.0 arm64; raw build output stays ignored under `build/launch-docs/`.

This evidence-only checkpoint does not change build inputs. A later workflow/toolchain/runtime change requires another clean build before a publication candidate; a final candidate must carry its selected sources and actual package hashes. These outputs remain local and unpublished.
