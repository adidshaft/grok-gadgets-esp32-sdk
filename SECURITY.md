# Security

Do not publish credentials, private Wi-Fi settings, household state or vulnerability exploit details in an issue.

After repository activation, use [GitHub private vulnerability reporting](https://github.com/adidshaft/grok-gadgets-esp32-sdk/security/advisories/new) when enabled. Until that channel is available, email [adidshaft@kyokasuigetsu.xyz](mailto:adidshaft@kyokasuigetsu.xyz). Describe the affected commit, component, evidence level, reproduction and likely impact privately. Omit secrets; agree a safe reproduction route before supplying sensitive logs. No response-time guarantee is advertised.

This alpha supports USB and Wi-Fi firmware. Restrict access to the host and the serial device.
USB credentials stay on the host bridge. Wi-Fi provisioning stores credentials on the board without flash encryption.
Wi-Fi uses TLS with a pinned gateway CA and a per-device token.
If you lose a board, revoke its token and change the Wi-Fi password.
Grok Bot reaches the gateway's MCP port through a tunnel the owner runs; the device ports are never tunnelled. Never expose the unencrypted loopback device listener to a network.

Use the hub's [disclosure process](https://github.com/adidshaft/grok-gadgets/blob/main/SECURITY.md). Routine build problems belong in [Support](SUPPORT.md). Security fixes must preserve retry uncertainty, device/session isolation and the distinction between acknowledgements and physical effects.
