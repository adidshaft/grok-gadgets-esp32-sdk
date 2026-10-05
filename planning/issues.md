# Local issue ledger

Canonical structured ledger: [issues.json](issues.json). Owner: grok-gadgets-esp32-sdk. Local authority until approved GitHub migration.

| ID | Problem | Stage | Evidence / dependency |
| --- | --- | --- | --- |
| ESP-001 | Independent open-source repository foundation | done | Foundation checks and Git diff review passed. |
| ESP-002 | Portable independently useful ESP32 SDK | done | C++ host tests and pinned canonical protocol frame validation passed. |
| ESP-003 | C124 USB firmware and lifecycle | done | 3/3 CTest; SDK frame contract; PTY actual firmware simulation/gateway execution, server restart and credential revocation/restoration; real firmware compile. No flash. |
| ESP-004 | Physical C124 acceptance | blocked | Blocker: no C124 board is available here, and flashing is not authorized. |
| ESP-005 | Secure Wi-Fi transport and provisioning | blocked | Blocker: gateway 0.1.0 has no authenticated LAN or TLS device endpoint to build on. |
| ESP-006 | Independent reproduction | blocked | Blocker: needs an independent person and a physical board (ESP-004). |
| ESP-007 | Undeclared overflow event causes reconnect loop | done | Declared history_lost; actual consumer 20-edge PTY regression retains 16/drop4, stays connected, executes next command; firmware rebuild passed |
| HARD-ESP-001 | Preserve ACKs across repeated retries | done | Reachable commits `851ffab` and `804f64f`. Host tests, contract, PTY four identical retry ACKs, and C124 compile passed on that integrated source. See [hardening evidence](../docs/verification/hardening.md). |

| LAUNCH-DOCS-ESP-001 | Standalone C124 SDK guide and public contributor/support policies | done | Documentation 304c518; fresh source/env pip installation, host3/3, contract and C124 compilation pass; clean build hashes recorded; [launch evidence](../docs/verification/launch.md). Physical/Grok pending. |
| REVIEW-ESP-001 | Reusable `grok::Gadget` session; event, loss, parse and registration fixes (review ESP-01/02/07/08/09/10/11/15/18/19) | done | Host session and firmware tests. ESP-08 RX buffer is set before `Serial.begin()` and remains unverified on hardware. |
| REVIEW-ESP-002 | Contract check validates real sketch output (ESP-14) | done | `transcript_c124` and `transcript_led_button` compile the real sketches; `check_contract.py` validates their frames. |
| REVIEW-ESP-003 | Optional PTY integration fails instead of hanging (ESP-06) | done | `tools/check_gateway.py` bounds close. Gateway `DeviceServer.close` closes clients first. |
| REVIEW-ESP-004 | Reproducible firmware hashes, one checksum record, flash offsets (ESP-05/16) | done | `docs/build-checksums.json` records clean `f1af1d4` on toolchain system `darwin_x86_64`. Path maps are on. Another host variant still changes `firmware.bin`. |
| REVIEW-ESP-005 | Plain README and corrected guides (ESP-13/15/16/19/21, enroll/serve) | done | One build quickstart, explicit Bot/hardware gates, local HTTP MCP, and same-port USB retry. Host 4/4, two generated sketch contracts, PTY simulation and both firmware builds pass. Physical USB and Grok Bot remain unverified. |
| REVIEW-ESP-006 | Ledger citations and blocker fields (ESP-17) | done | `851ffab` / `804f64f`. ESP-004/005/006 name external blockers. |
| REVIEW-ESP-007 | Deferred: boot_id counter (ESP-12), float RGB (ESP-03, gateway), protocol request IDs | ready | Needs NVS work or coordinated protocol/gateway changes. |
| REVIEW-ESP-008 | Pin protocol 0.1.0 to gateway 5ea23b7 (events + ID anchors) | done | Copied files; check_contract.py |
