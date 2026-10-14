# ESP32-S3 OLED kit

Firmware for the ESP32-S3-N16R8 DevKitC-style board (YD-ESP32-23 v1.3) with a 1.3-inch 128×64 I²C OLED.
It passed physical acceptance on the owner's kit on 12 October 2026
([record](../../docs/verification/oled-kit-2026-10-12.md)).

| Command | Effect |
| --- | --- |
| `display.text` | Shows 1–32 printable ASCII characters, 16 per line. Other text fails and the screen stays unchanged. |
| `display.clear` | Blanks the screen. |
| `rgb.set` | Sets the onboard RGB pixel (GPIO48). |
| `button` event | Press and release of the BOOT button, read after boot. |

The state reports `display.ready`, the drawn text, and the I²C pins and address found at boot.
It reports what the firmware drew, not what anyone saw.

## Wiring

Unplug USB first. Use the board's **USB** port, not **COM**.

| OLED pin | Board pin |
| --- | --- |
| VCC | 3V3 |
| GND | GND |
| SCK | GPIO9 |
| SDA | GPIO8 |

The firmware also finds the display with SCK and SDA swapped.
The delivered module is an SH1106. For an SSD1306 module, add `-DOLED_KIT_SSD1306`.
For a board whose RGB pixel is on GPIO38, add `-DOLED_KIT_RGB_PIN=38`.

## Build and flash

```sh
.venv/bin/pio run -e esp32s3-oled-kit
.venv/bin/pio run -e esp32s3-oled-kit -t upload
```

If the first upload cannot connect, hold BOOT, plug the cable into **USB**, wait two
seconds and release BOOT. Then upload again.

## Run without hardware

The software model checks the same commands without a board:

```sh
python3 examples/oled-kit/rehearse.py
python3 -m unittest discover -s examples/oled-kit -p 'test_*.py' -v
```

The model emits JSON with `simulated: true`. It uses no GPIO, USB or credentials.

## Wi-Fi build

The `esp32s3-oled-kit-wifi` environment runs the same commands over your home Wi-Fi, so
the board needs only a USB charger. Keep the Mac gateway running. This is not a computer-free system.
The design is hub
[ADR 005](https://github.com/adidshaft/grok-gadgets/blob/dev/docs/decisions/005-wifi-device-route.md).
The gateway setup is in its [Wi-Fi devices guide](https://github.com/adidshaft/grok-gadgets-gateway/blob/dev/docs/wifi-devices.md).
It passed physical acceptance over Wi-Fi on the owner's kit on 12 October 2026
([record](../../docs/verification/oled-kit-wifi-2026-10-12.md)).

- The board connects with TLS to the gateway's LAN listener (`serve --lan-listen`). It trusts
  only the gateway CA that provisioning stored. It never uses a public CA bundle or skips
  verification.
- The hello carries the device token. The frames are the same protocol 0.1.0 frames as USB.
- The gateway address can be its mDNS name, for example `my-mac.local`, as in
  `wifi-provision ... --lan my-mac.local`. Before each connection the board looks the name up and
  checks the certificate against the name. If the computer's address changes, the board follows it.
  If the name does not answer, the top row shows `Gateway name?`. The board also announces its own
  name, its device ID plus `.local`, on the home network.
  On the owner's kit this passed two address changes and a Mac restart
  ([record](https://github.com/adidshaft/grok-gadgets-gateway/blob/main/docs/verification/always-on-2026-10-13.md)).
- The gateway can also be a relay on an always-on host, reached by its DNS name. Provision
  with `wifi-provision ... --gateway devices.example.net` and the relay's CA
  ([relay host guide](https://github.com/adidshaft/grok-gadgets-gateway/blob/dev/docs/relay-host.md),
  hub [ADR 009](https://github.com/adidshaft/grok-gadgets/blob/dev/docs/decisions/009-relay-route.md)).
  The board looks the name up before each connection and checks the certificate against it.
  The top row shows `Relay...` while it connects and `Relay name?` if the name does not resolve.
  The name may have up to 63 characters. The owner's kit passed this route on 13 October 2026
  ([record](https://github.com/adidshaft/grok-gadgets/blob/dev/docs/verification/mac-free-route-2026-10-13.md)).
- The ESP32-S3 joins 2.4 GHz networks only.
- The top OLED row shows the link: `Not provisioned`, `WiFi...`, `Gateway...` (or `Relay...`),
  `Online`, `TLS rejected` or `Token rejected`. Rows 2–5 show the text. `display.clear` blanks the
  text and keeps the status row.
- While the status says `Online`, the second row says `Grok Bot`. A 48×48 Grok Bot face
  fills the right side of the first six rows. It moves one pixel up and down and blinks once
  every 3.6 seconds. Text then uses the bottom two rows, 16 columns each, by word when two
  lines are enough. A failure, connection message or reset countdown replaces the face and
  label, and text goes back to rows 2 and 4. `display.clear` keeps the face and labels.
- If the board cannot join Wi-Fi, the top row says why and the second row says what to
  check. The console prints the same reason and answers `status` with `wifi_failure` and
  `wifi_hint`. Neither ever shows the network name or password.

  | Top row | Second row | Cause | Fix |
  | --- | --- | --- | --- |
  | `No network` | `2.4GHz? Name?` | The board cannot see the network name. | Use a 2.4 GHz network; the ESP32-S3 has no 5 GHz radio. Check the name's upper and lower case. Provision again. |
  | `Wrong password` | `Check password` | The network refused the password. | Provision again with the correct password. |
  | `Join timeout` | `Check router` | A join attempt got no answer for 15 seconds. | Check that the router is on and the board is in range. |

  The wrong-password case passed on the owner's kit
  ([record](../../docs/verification/wifi-join-failure-2026-10-12.md)). The other two are host-tested only.
- After Wi-Fi or gateway loss, the board reconnects with a fresh hello. The wait starts at
  0.5 s and doubles up to 5 s. Each wait is cut by up to a quarter at random, never below 0.5 s.
  Boards that lost the same relay then do not all reconnect at once. After a Wi-Fi drop, a join
  counts as timed out only after 15 s. After `unauthorized` or `revoked` it waits 60 s. A command is
  never replayed into a new session.

The pixel face follows the project's
[Grok Bot reference](https://github.com/adidshaft/grok-gadgets/blob/dev/website/media/grok-bot-mark.svg).
It is a small display interpretation of the user-supplied artwork, not an official logo asset.
The animation writes tiles only when the face changes, with no added delay or full-screen refresh.
On 12 October 2026 the owner checked the 48×48 face and `Grok Bot` label on the kit
(`dev` `cedeeff`). Text on rows 6 and 7 beside it is host-tested only.

### Flash and provision

Provisioning uses the USB data cable once. Stop any `usb-bridge` first.

```sh
.venv/bin/pio run -e esp32s3-oled-kit-wifi -t upload
grok-gadgets-gateway wifi-provision /dev/cu.usbmodem101 --device oledkit-0123456789ab --lan 192.168.1.20
```

`wifi-provision` prompts for the Wi-Fi name and password, without echo for the password. It
sends them with the device token, the listener address and the gateway CA in one line on
the serial console. The board stores them in NVS namespace `grok-wifi` and restarts. The
console never prints a stored value.

The serial console in this build takes one-line JSON commands:

| Command | Reply |
| --- | --- |
| `{"type":"status"}` | Device ID, firmware, `provisioned`, `factory`, Wi-Fi and gateway state |
| `{"type":"provision", ...}` | `ok` and `restarting`, or `invalid_settings` without any value |
| `{"type":"factory_provision", ...}` | Writes the factory identity (see below); `already_provisioned` unless `replace` is true |
| `{"type":"factory_reset"}` | Erases the user's settings (Wi-Fi) and restarts; the factory identity stays |

Lines that start with `status:` report link changes as text.

### Factory identity and user settings

The board keeps two NVS namespaces (hub ADR 010):

- `grok-factory`: the device token, the relay name and port, the CA, and the setup hotspot's
  passphrase and setup code. The builder writes it once with `factory_provision`
  (`grok-gadgets-gateway factory-provision`).
- `grok-wifi`: the user's settings, the home Wi-Fi name and password.

A factory reset erases `grok-wifi` only, so the board keeps its identity. A full `provision`
line also writes `grok-wifi`; its token, address and CA then take precedence over the
factory identity. A board provisioned before ADR 010 has no `grok-factory` and keeps working.

### Factory reset

Hold BOOT for 10 seconds, or run `wifi-provision PORT --device ID --factory-reset`.
After 3 seconds of holding, the top row counts down from `Reset in 7s`. Let go before it
reaches zero to keep the settings. The
board erases its Wi-Fi settings and restarts as `Not provisioned`. A board with a factory
identity keeps it. For a board without one, also run `grok-gadgets-gateway revoke ID`. `esptool erase_flash` erases everything.

### Limits

- Flash encryption is off. Whoever holds the board can read the Wi-Fi password and the token
  from flash. If you lose the board, revoke its token and change the Wi-Fi password.
- The board has no clock and does not check certificate dates. Replace trust with
  `lan-ca rotate` and provision again.
- The board knows the gateway by its IP address. Reserve that address on the router, or
  provision again after it changes.
