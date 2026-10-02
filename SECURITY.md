# Security

Do not publish credentials, private Wi-Fi settings, household state or vulnerability exploit details in an issue.

After repository activation, use [GitHub private vulnerability reporting](https://github.com/adidshaft/grok-gadgets-esp32-sdk/security/advisories/new) when enabled. Until that channel is available, email [adidshaft@kyokasuigetsu.xyz](mailto:adidshaft@kyokasuigetsu.xyz). Describe the affected commit, component, evidence level, reproduction and likely impact privately. Omit secrets; agree a safe reproduction route before supplying sensitive logs. No response-time guarantee is advertised.

This experimental alpha includes local USB firmware and bounded command/event handling. Host and USB access can control the connected gadget; restrict access to the computer and serial device. Per-device credentials stay on the host bridge, outside Git and firmware. Secure Wi-Fi/provisioning and an authenticated remote Grok route are unimplemented and require separate review. Do not expose the existing loopback device listener as a public service.

Use the hub's [disclosure process](https://github.com/adidshaft/grok-gadgets/blob/main/SECURITY.md). Routine build problems belong in [Support](SUPPORT.md). Security fixes must preserve retry uncertainty, device/session isolation and the distinction between acknowledgements and physical effects.
