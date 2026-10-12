# Ordered OLED kit

Firmware for the ESP32-S3-N16R8 DevKitC-style board with a 0.96-inch 128×64 I²C OLED.
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
