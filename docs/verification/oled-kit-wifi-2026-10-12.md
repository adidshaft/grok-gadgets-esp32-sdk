# Physical acceptance over Wi-Fi: ESP32-S3 OLED kit — 12 October 2026

The owner ran this test on their own kit, at home, with the gateway on their Mac. The
owner observed each physical result. The gateway request log (`serve --request-log`)
recorded each connection and command. This record covers this one kit, one router and one
host. It does not prove that every board or network works. Times are UTC.

The design is hub [ADR 005](https://github.com/adidshaft/grok-gadgets/blob/dev/docs/decisions/005-wifi-device-route.md).
The USB acceptance of the same kit is in [oled-kit-2026-10-12.md](oled-kit-2026-10-12.md).

## Setup

| Part | Detail |
| --- | --- |
| Kit | YD-ESP32-23 v1.3 (ESP32-S3-N16R8), SH1106 OLED at 0x3c on GPIO8/9, BOOT on GPIO0. Device `oledkit-84c7bb57148c`. |
| Power | A USB charger for every acceptance item. The Mac's USB port was used only to flash and provision. |
| Network | The owner's home router, 2.4 GHz, WPA2 Personal. The board took a private address from the router. |
| Gateway | `serve --lan-listen` on the Mac's private Wi-Fi address, port 8767, TLS from the local CA that `init` created. Gateway `dev` `0e51fdd`, then `e6bfe6a` from item 4 on. macOS 27.0. |
| Firmware | `esp32s3-oled-kit-wifi`, firmware version string `0.2.0-wifi`. Image A: `dev` `3da4a34`, `firmware.bin` SHA-256 `9b5a953f1eba11485180f36abb2fcc6d5fffc23d60b044e9778b9af00283ff42`. Image B: `dev` `d128a33`, `firmware.bin` SHA-256 `7283685c546e7ea9b8398f50a80670046b16e00b1e0647fc5f485ec840dd0e5f`. esptool verified each write. Image B was written to the app area only, so NVS kept its settings. |
| Provisioning | `grok-gadgets-gateway wifi-provision` over USB. The owner typed the Wi-Fi password into its local prompt. The password and token appear in no log, chat or file in this record. |

## Results

| # | Check | Result | Owner observation | Gateway request log |
| --- | --- | --- | --- | --- |
| 1 | Connect on boot | **Pass** (image A) | On the charger the top row showed **Online**. | 11:17:32.398 `lan_connected`, 11:17:32.412 `lan_hello` `oledkit-84c7bb57148c`, peer on the home network |
| 2 | Text and clear | **Pass** (image A) | Showed "Hello over Wi-Fi" under **Online**; after clear only **Online** remained. | 11:17:50.857 `display.text` `executed`, `simulated: false`; 11:18:34.680 `display.clear` `executed` |
| 3 | BOOT events | **Pass** (image A) | Pressed BOOT; later one quick press. | 11:20:52.390 `button` pressed, 11:21:02.246 released (a hold of about 10 s); 11:30:12.359 pressed, 11:30:12.534 released (0.17 s), same boot |
| 4a | Wi-Fi drop and recovery | **Fail** (image A) | Router off 3–4 minutes, then on. The Mac and other devices reconnected; the board stayed at **WiFi...**. | 11:37:33.048 `lan_disconnected`; no connection attempt in the next 5 minutes |
| 4b | Wi-Fi drop and recovery, retest | **Pass** (image B) | Router off, then on. Back to **Online** in under 15 s after the router came back. Showed "Back after Wi-Fi drop". | 11:58:35.960 `lan_disconnected`; 12:00:02.813 `lan_connected`, 12:00:03.784 `lan_hello`; 12:00:43.262 `display.text` `executed` |
| 5 | Gateway restart | **Pass** (image B) | Showed "Gateway restarted". Whether the top row showed **Gateway...** during the 14 s outage was not reported. | 12:01:06.270 `serve_stopped`; 12:01:19.946 `serve_started`; 12:01:24.805 `lan_connected`, 12:01:24.824 `lan_hello`; 12:01:32.091 `display.text` `executed` |
| 6 | Token revoke, then re-enroll | **Pass** (image B) | Showed **Token rejected**; after provisioning the new token over USB, **Online** again. | `revoke` at 12:02:42.108; 12:02:42.532 `lan_auth_rejected` `revoked` and `lan_disconnected` (0.42 s); next attempt 12:03:43.642 `revoked` (61 s later); `enroll --rotate` at 12:03:51; 12:05:37.058 `lan_auth_rejected` `unauthorized` (old token); 12:06:06.514 `lan_hello` with the new token |
| 7 | Wrong-CA rejection | **Pass** (image B) | Showed **TLS rejected**; **Online** again after the real certificate returned. | Gateway restarted with a certificate from a throwaway CA for the same address: 12:07:17.656 `serve_started`; 23 × `lan_tls_failed` `TLSV1_ALERT_UNKNOWN_CA` from 12:07:18.825 to 12:09:08.820, about one every 5 s, no `lan_hello`. Real certificate: 12:09:15.629 `serve_started`, 12:09:19.494 `lan_hello` |
| 8 | Power cycle | **Pass** (image B) | Charger out and in: **WiFi...**, **Gateway...**, **Online**. | 12:10:33.724 `lan_disconnected`; 12:10:37.069 `lan_connected`, 12:10:37.082 `lan_hello`; new boot ID, displayed text empty |
| — | Factory reset by holding BOOT (unplanned) | **Pass** (image A) | Held BOOT about 10 s; the board showed **Not provisioned**. | 11:21:11.407 `button` pressed, no release; 11:21:36.280 `lan_disconnected`. Provisioned again over USB; 11:29:27.889 `lan_hello` |

Items 1–3 ran on image A. Image B changes only how the board rejoins Wi-Fi. Text commands
were checked again on image B (items 4b, 5 and the Grok Bot run). BOOT events were not
repeated on image B.

## Grok Bot

**Pass (local route, no approval card).** The board was on the charger (image B). A message
went into a new chat in the owner's Grok Bot desktop app on this Mac at 17:43:05 IST
(12:13:05 UTC). It asked Grok Bot to run this command:
`grok-gadgets-gateway rehearse --device oledkit-84c7bb57148c --command display.text --args '{"text":"Hi from Grok Bot on Wi-Fi"}'`.
A computer-use helper sent the message on the owner's behalf; the owner directed it.
No approval card appeared: Execution on Local Computer was still **Always allow**.
The chat showed the command output, including `gadgets_command display.text … -> executed`.
It also showed a line beginning `Rehearsal passed`; the chat view cut off the rest of it.
No screenshot file was saved.

The gateway log recorded `gadgets_list_devices`, `gadgets_command display.text`
(`executed`, `simulated: false`, text "Hi from Grok Bot on Wi-Fi") and `gadgets_get_state`
at 12:13:20 UTC from client `grok-gadgets-rehearse`. The board received the command over its
Wi-Fi TLS session (`lan_hello` 12:10:37). The owner saw "Hi from Grok Bot on Wi-Fi" on the
OLED under **Online**.

This is the documented local route, outcome (b) of the hub's ADR 004. Grok Bot ran a shell
command; it did not call the MCP tools itself. The link to Grok Bot is the chat and the
matching time.

## Problems found and fixed

- **No rejoin after a long router outage** (item 4a). The Arduino-ESP32 core's automatic
  reconnect stopped retrying. The firmware now starts a fresh join every 15 s while offline
  ([#74](https://github.com/adidshaft/grok-gadgets-esp32-sdk/pull/74)).
- **Gateway traceback on a vanished device.** Closing a TLS socket whose peer lost Wi-Fi
  raised `SSL shutdown timed out` after 30 s. The gateway now waits at most 2 s and aborts the
  socket ([gateway #81](https://github.com/adidshaft/grok-gadgets-gateway/pull/81)). No
  traceback appeared after the fix.
- **Wi-Fi name case.** The first provisioning used the network name in the wrong case, and
  the board stayed at **WiFi...**. Network names are case-sensitive.
- **5 GHz.** The Mac was first on a 5 GHz channel. The ESP32-S3 joins 2.4 GHz only.
- **The Mac's address changed** when it moved to 2.4 GHz. `lan-cert` was run again for the
  new address and the board was provisioned again. A DHCP reservation for the Mac avoids this.
- **Blank screen after loose jumpers.** Jumpers came off while the charger was changed. The
  firmware probes the display only at boot, so the screen stayed blank until a restart with
  the jumpers seated.
- **A 10 s BOOT hold is easy to reach by accident.** The owner held BOOT while waiting for
  feedback and reset the board. A warning before the reset is a follow-up.
- **No reason shown for a failed join** — follow-up
  [#73](https://github.com/adidshaft/grok-gadgets-esp32-sdk/issues/73).

## Not covered

One kit, one router, one Mac. No independent reproduction, no Linux host, no other access
point. Disconnect during a command was not tested. The listener's per-address rate limit
was not reached (the board retried about 12 times a minute). Flash encryption is off, as ADR
005 states. End-user setup without a USB cable is
[#72](https://github.com/adidshaft/grok-gadgets-esp32-sdk/issues/72).
