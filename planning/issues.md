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

| LAUNCH-DOCS-ESP-001 | Standalone C124 SDK guide and public contributor/support policies | done | Documentation 9177ff5; fresh source/env pip installation, host3/3, contract and C124 compilation pass; clean build hashes recorded; [launch evidence](../docs/verification/launch.md). Physical/Grok pending. |
