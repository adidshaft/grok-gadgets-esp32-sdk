# Climate sensor

An AHT20 temperature and humidity module on a Wi-Fi gadget. Grok Bot can answer "how warm is it?"
and "tell me when it goes above 26 degrees". The design is hub
[ADR 013 §6](https://github.com/adidshaft/grok-gadgets/blob/main/docs/decisions/013-builder-kit.md).

**Not tested on hardware.** The sketch passes host tests and compiles for the board; nobody has
run it with the part yet.

| Name | Kind | What it does |
| --- | --- | --- |
| `climate` | State | `temperature_c` and `humidity_pct` to 0.1, `above_c`, and `sensor`: `ok` or `missing` |
| `threshold.set` | Command | `{"above_c": number}` from -40 to 85. Other values are refused and change nothing |
| `threshold.crossed` | Event | The temperature went above or below `above_c`: `reading`, `value`, `above` |
| `button` | Event | The BOOT button. Holding it 10 seconds is a factory reset |

The module is read every 2 seconds. Setting a threshold sends no event by itself. A module that
is missing at boot, or stops answering three times in a row, reports `sensor: missing` and has no
reading; the sketch looks for it again every 5 seconds.

## Parts

- ESP32-S3-DevKitC-1 with an ESP32-S3-WROOM-1-N16R8 module (16 MB flash, 8 MB octal PSRAM).
- An AHT20 I²C module that runs on 3.3 V. An AHT20 + BMP280 board also works; only the AHT20 is
  read. AHT10, DHT11, DHT22, SHT31 and BME280 need other code.
- Four jumper wires.

## Wiring

Unplug USB first.

| AHT20 pin | Board pin |
| --- | --- |
| VCC | 3V3 |
| GND | GND |
| SDA | GPIO8 |
| SCL | GPIO9 |

GPIO8 and GPIO9 are the OLED kit's display pins, so the module can share that bus (address 0x38).

## Run without hardware

The host build runs this sketch with a simulated module that warms from 22 to 28 degrees:

```sh
sh tools/check.sh
.venv/bin/python examples/climate-sensor/rehearse.py
```

It prints each acknowledgement and the `threshold.crossed` event. This is host evidence only.

## Build and first image

Follow [Ship a gadget](../../docs/ship-a-gadget.md) with `esp32s3-climate-sensor`. A new
board's first image goes on by USB; later images can go by signed update for the same
environment name.

## Hardware check

With the gadget Online and claimed, ask Grok Bot:

1. "How warm is it where the climate sensor is?" Expect a temperature and humidity close to a
   room thermometer, and `sensor: ok`.
2. "Tell me when it goes above" a value 1 degree above the reading. Warm the module with a
   fingertip. Expect `threshold.crossed` with `above: true`, then `above: false` as it cools.
3. "Set the threshold to 100." Expect a refusal and an unchanged `above_c`.
4. Unplug VCC. Within about 10 seconds the state says `sensor: missing`. Plug it back: a reading
   returns within about 5 seconds.
