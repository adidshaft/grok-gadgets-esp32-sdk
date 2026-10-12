# Wi-Fi join failure reason: wrong password — 12 October 2026

The owner ran this check on the same kit as the
[Wi-Fi acceptance record](oled-kit-wifi-2026-10-12.md), with the gateway on their Mac. The
owner observed the OLED. The console reply and the gateway request log are below. Times are
UTC. This record covers one wrong-password check on one kit and one router. `No network` and
`Join timeout` were not tested on hardware.

## Setup

| Part | Detail |
| --- | --- |
| Kit | YD-ESP32-23 v1.3 (ESP32-S3-N16R8), SH1106 OLED on GPIO8/9. Device `oledkit-84c7bb57148c`. Power and console over the board's `USB` port. |
| Gateway | `serve --lan-listen` on the Mac's private address, unchanged from the acceptance run. |
| Firmware | `esp32s3-oled-kit-wifi`, written to the app area only (`0x10000`), so NVS kept the stored settings. esptool verified each write. |
| Provisioning | `grok-gadgets-gateway wifi-provision` in the owner's terminal. The owner typed the network name and the passwords at its prompts. No name or password appears in this record. |

## Found first: provisioning crashed the board

Image 1 was the first commit of this change (`645b55c`, `firmware.bin` SHA-256
`1106dbebaee81019e86a9cbaeb3bda0ab44c8895058dbea4397b2f157e04ba9a`). `wifi-provision` failed
with "The device did not answer". A test line that the board must refuse as invalid showed
the cause on the console:

```text
Guru Meditation Error: Core  1 panic'ed (Unhandled debug exception).
Debug exception reason: Stack canary watchpoint triggered (loopTask)
```

The board restarted with its old settings, so nothing changed. `-fstack-usage` frame sizes:

| Build | `loop` | `Console::handle` | `readSettings` |
| --- | --- | --- | --- |
| `v0.2.0-alpha.9` | 96 B | 4064 B | 3504 B |
| Image 1 | 272 B | 4064 B | 3504 B |
| Image 2 | 272 B | 704 B | inlined |

The loop task has 8 KB. Image 1 added 176 bytes to `loop`, and the provisioning path went
over the limit. `v0.2.0-alpha.9` is within about 500 bytes of it. Nobody has provisioned a
board with `alpha.9`; the Phase 4 images that were provisioned had a smaller `loop`.
Image 2 (`eabf495`) keeps the settings buffer in the console object instead of on the stack.

## Result with image 2

Image 2: `eabf495`, `firmware.bin` SHA-256
`9454fdea7b7a914b5e5d1ae098530c1d0b482ee921472a3a11f61d3a952c62ee`.

| # | Check | Result | Evidence |
| --- | --- | --- | --- |
| 1 | An invalid provision line gets a reply, no restart | **Pass** | Reply `invalid_settings` ("Wi-Fi password must be empty or 8-64 bytes"); the next `status` reply still said `Online`. |
| 2 | Wrong password: OLED | **Pass** | The owner saw **Wrong password** on the top row and **Check password** on the second row. |
| 3 | Wrong password: console | **Pass** | `status` reply: `"wifi": "disconnected"`, `"gateway": "Wrong password"`, `"wifi_failure": "wrong_password"`, `"wifi_hint": "The network rejected the password. Provision the board again."` No network name or password in the reply. |
| 4 | Wrong password: gateway | **Pass** | No `lan_hello` from the kit between 15:13:35.614 (`lan_disconnected`) and 15:15:54. |
| 5 | Real password restored | **Pass** | The owner saw **Gateway...** briefly, then **Online**. 15:15:54.343 `lan_connected`, 15:15:54.362 `lan_hello` `oledkit-84c7bb57148c`. |

The board reported `Wrong password` from the station's disconnect reason. This router and
firmware gave one of the codes mapped to it (15, 202 or 204); the exact code was not logged.
