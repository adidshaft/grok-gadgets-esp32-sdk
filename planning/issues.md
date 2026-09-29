# Local issue ledger

Owner: grok-gadgets-esp32-sdk. Local authority until approved GitHub migration.

| ID | Problem / acceptance | Labels | Milestone | Stage | Evidence / dependency |
| --- | --- | --- | --- | --- | --- |
| ESP-001 | Independent Apache-2.0 repository and contribution foundation | maintenance, esp32, P1 | M0 | done | License, contributor/security/agent instructions and templates; no public repository |
| ESP-002 | Portable capability, RGB validation, debounce, framing and retry tests | feature, esp32, protocol, P1 | M4 | done | CTest 2/2; strict RGB/custom capability/dedup, debounce/wrap/framing; pinned protocol fixture validation |
| ESP-003 | C124 USB firmware compiles; bridge and recovery docs | feature, esp32, P1 | M4 | ready | Requires official pins; real compile and host integration |
| ESP-004 | Physical LED/button/unplug/reboot acceptance | feature, esp32, P1, needs hardware, help wanted | M8 | blocked | No board; requires C124 and user authorization to flash |
| ESP-005 | Wi-Fi transport using same contract and secure provisioning | feature, esp32, P2 | M4 | proposed | Depends on USB and authenticated reachable LAN/TLS gateway route |
| ESP-006 | Independent clean setup and physical reproduction | docs, esp32, P2, help wanted | M8 | blocked | Requires another tester |
