# Local issue ledger

Canonical structured ledger: [issues.json](issues.json). Owner: grok-gadgets-esp32-sdk. Local authority until approved GitHub migration.

| ID | Problem | Stage | Evidence / dependency |
| --- | --- | --- | --- |
| ESP-001 | Independent open-source repository foundation | done | Foundation checks and Git diff review passed. |
| ESP-002 | Portable independently useful ESP32 SDK | done | C++ host tests and pinned canonical protocol frame validation passed. |
| ESP-003 | C124 USB firmware and lifecycle | done | 3/3 CTest; SDK frame contract; PTY actual firmware simulation/gateway execution, server restart and credential revocation/restoration; real firmware compile. No flash. |
| ESP-004 | Physical C124 acceptance | blocked | No C124 available; requires physical board and authorization to flash. |
| ESP-005 | Secure Wi-Fi transport and provisioning | blocked | Canonical gateway 0.1.0 binds loopback-only, without LAN/TLS endpoint; plain network credentials/exposure excluded. Secure endpoint/provisioning contract required. |
| ESP-006 | Independent reproduction | blocked | Independent tester and physical board unavailable. |
| ESP-007 | Undeclared overflow event causes reconnect loop | done | Declared history_lost; actual consumer 20-edge PTY regression retains 16/drop4, stays connected, executes next command; firmware rebuild passed |
| HARD-ESP-001 | Preserve ACKs across repeated retries | done | Fixad89e7a, canonical pin08bfa69; clean main CTest3/3, contract, PTY four identical retry ACKs and C124 compile pass. Rebuilt clean-source artifacts and hashes/toolchain verified. See [hardening evidence](../docs/verification/hardening.md). |

| LAUNCH-DOCS-ESP-001 | Standalone C124 SDK guide and public contributor/support policies | done | Documentation 304c518; fresh source/env pip installation, host3/3, contract and C124 compilation pass; clean build hashes recorded; [launch evidence](../docs/verification/launch.md). Physical/Grok pending. |
| REVIEW-ESP-001 | Reusable `grok::Gadget` session; event, loss, parse and registration fixes (review ESP-01/02/07/08/09/10/11/15/18/19) | in-progress | Host session/firmware tests and both sketch builds. ESP-08 RX buffer unverified on hardware. |
| REVIEW-ESP-002 | Contract check validates real sketch output (ESP-14) | in-progress | Transcript binaries compile the real sketches. |
| REVIEW-ESP-003 | Optional PTY integration fails instead of hanging (ESP-06) | in-progress | Gateway close deadlock is fixed in the gateway repository. |
| REVIEW-ESP-004 | Reproducible firmware hashes, one checksum record, flash offsets (ESP-05/16) | in-progress | Host toolchain variant still changes the hash. |
| REVIEW-ESP-005 | Plain README and corrected guides (ESP-13/15/16/19/21, enroll/serve) | in-progress | Documentation only. |
| REVIEW-ESP-006 | Ledger citations and blocker fields (ESP-17) | in-progress | Ledger only. |
| REVIEW-ESP-007 | Deferred: boot_id counter (ESP-12), float RGB (ESP-03, gateway), protocol request IDs | ready | Needs NVS work or coordinated protocol/gateway changes. |
