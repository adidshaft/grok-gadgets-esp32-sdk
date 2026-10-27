# Ship a gadget: build, flash and prepare a batch

This page covers the firmware part of shipping gadgets that you built. The hub guide
"Ship a gadget" covers the whole afternoon, from the relay to hand-over. Read
[Before you distribute a gadget](https://github.com/adidshaft/grok-gadgets/blob/main/docs/getting-started/before-you-distribute.md)
before you give or sell a gadget.

## 1. Pick a starting point

| Example | What it does | Part |
| --- | --- | --- |
| `examples/oled-kit` | Shows text from Grok Bot | 1.3-inch I2C OLED (owner-tested) |
| [`examples/climate-sensor`](../examples/climate-sensor/README.md) | Reports temperature and humidity; tells Grok Bot when it gets hot | AHT20 on I2C |
| [`examples/switch`](../examples/switch/README.md) | Turns a low-voltage load on or off | 3.3 V opto-isolated relay module |
| [`examples/servo`](../examples/servo/README.md) | Moves to an angle, for example a feeder flap | SG90 servo |

The climate sensor, switch and servo examples use `grok::WifiApp` (`GrokApp.h`). It brings
the relay link, phone setup, claim codes, signed updates and the factory-reset button. A
gadget without a display shows its claim code on the setup page after it goes Online.

These three examples have host models, host tests and a `rehearse.py` that runs the sketch on
your computer. Each README lists the parts, the wiring and the checks to make with Grok Bot.
**Nobody has run them with the parts yet.** Treat them as Not tested on hardware until you do.

**Never wire mains voltage to the switch example.**

The switch is for low-voltage loads only, such as a 5 V or 12 V DC lamp or fan.

## 2. Build with your update key

Make your signing key once (gateway `firmware keygen`). Then build every image with its public
key, so that you can update the gadgets later without a cable:

```sh
GROK_UPDATE_PUBLIC_KEY=~/grok-gadgets-keys/builder.pub.pem \
  .venv/bin/pio run -e esp32s3-climate-sensor
```

`GROK_FIRMWARE_VERSION` sets the version that the gadget reports. Sign each later image with
the same version and with the environment name as the board (see [updates](updates.md)).

## 3. Flash each board over USB

```sh
GROK_UPDATE_PUBLIC_KEY=~/grok-gadgets-keys/builder.pub.pem \
  .venv/bin/pio run -e esp32s3-climate-sensor -t upload --upload-port /dev/cu.usbmodem101
```

A board from the factory may need BOOT held while you plug it in, the first time.

## 4. Read the IDs and prepare the batch

With each board on USB, read its device ID and list them in a CSV:

```sh
grok-gadgets-gateway factory-provision /dev/cu.usbmodem101 --read-id
grok-gadgets-gateway enroll --batch ids.csv --out batch-1
```

Then write each board's identity and label from the batch:

```sh
grok-gadgets-gateway factory-provision /dev/cu.usbmodem101 \
  --gateway devices.example.net --batch batch-1
```

Put each printed label on its own gadget. The label is a secret: keep the batch folder
private until every row says `provisioned: yes`.

## 5. Check before you pack

- On power-up without home Wi-Fi, the gadget opens its setup hotspot (`status: Setup 10 min`
  on the USB console).
- `grok-gadgets-gateway builder report` on the relay host lists the gadget after it first
  goes Online.

## Cost on the board

Arduino-ESP32 2.0.17, without an update key, 27 October 2026:

| Example | Static RAM (bytes) | Flash (bytes) |
| --- | ---: | ---: |
| `esp32s3-climate-sensor` | 108,068 | 927,193 |
| `esp32s3-switch` | 107,724 | 911,593 |
| `esp32s3-servo` | 107,788 | 917,021 |

An update key adds about 1.1 KB of RAM and 9.5 KB of flash.
