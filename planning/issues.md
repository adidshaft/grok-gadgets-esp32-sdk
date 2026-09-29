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
