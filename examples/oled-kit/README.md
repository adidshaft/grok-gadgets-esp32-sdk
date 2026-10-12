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
- The ESP32-S3 joins 2.4 GHz networks only.
- The top OLED row shows the link: `Not provisioned`, `WiFi...`, `Gateway...`, `Online`,
  `TLS rejected` or `Token rejected`. Rows 2–5 show the text. `display.clear` blanks the
  text and keeps the status row.
- After Wi-Fi or gateway loss, the board reconnects with a fresh hello. The wait starts at
  0.5 s and doubles up to 5 s. After `unauthorized` or `revoked` it waits 60 s. A command is
  never replayed into a new session.

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
| `{"type":"status"}` | Device ID, firmware, `provisioned`, Wi-Fi and gateway state |
| `{"type":"provision", ...}` | `ok` and `restarting`, or `invalid_settings` without any value |
| `{"type":"factory_reset"}` | Erases the settings and restarts |

Lines that start with `status:` report link changes as text.

### Factory reset

Hold BOOT for 10 seconds, or run `wifi-provision PORT --device ID --factory-reset`.
After 3 seconds of holding, the top row counts down from `Reset in 7s`. Let go before it
reaches zero to keep the settings. The
board erases its Wi-Fi settings and restarts as `Not provisioned`. Also run
`grok-gadgets-gateway revoke ID`. `esptool erase_flash` erases everything.

### Limits

- Flash encryption is off. Whoever holds the board can read the Wi-Fi password and the token
  from flash. If you lose the board, revoke its token and change the Wi-Fi password.
- The board has no clock and does not check certificate dates. Replace trust with
  `lan-ca rotate` and provision again.
- The board knows the gateway by its IP address. Reserve that address on the router, or
  provision again after it changes.
