# Low-voltage switch

**Never wire mains voltage (wall power) to this example.** Mains wiring needs an approved product
and a qualified person. This sketch is for low-voltage DC loads only, such as a 5 V USB lamp or
a 12 V fan.

A relay module on a Wi-Fi gadget. Grok Bot can turn the load on and off. The design is hub
[ADR 013 §6](https://github.com/adidshaft/grok-gadgets/blob/main/docs/decisions/013-builder-kit.md).

**Not tested on hardware.** The sketch passes host tests and compiles for the board; nobody has
run it with the part yet.

| Name | Kind | What it does |
| --- | --- | --- |
| `switch` | State | `on`, and `off_reason: link_lost` after a safe-off |
| `switch.set` | Command | `{"on": true}` or `{"on": false}`. Sending the same value again changes nothing |
| `button` | Event | The BOOT button. Holding it 10 seconds is a factory reset |

The load is off at boot. When the relay link has been down for 30 seconds, the load turns off by
itself, so a lost connection never leaves it on.

## Parts

- ESP32-S3-DevKitC-1 with an ESP32-S3-WROOM-1-N16R8 module.
- A 1-channel relay module with an optocoupler that switches from 3.3 V logic, for example one with
  an SRD-03VDC-SL-C relay. If it has an H/L jumper, set L. For a module or MOSFET board that
  switches on when its input is HIGH, build with `-DSWITCH_ACTIVE_HIGH`.
- A low-voltage load with its own supply: a 5 V USB lamp with a USB-A screw-terminal adapter, or a
  12 V DC fan with a 12 V adapter and a barrel-jack screw terminal.
- Jumper wires.

## Wiring

Unplug USB and the load's supply first.

| Relay module pin | Board pin |
| --- | --- |
| IN | GPIO5 |
| VCC | 3V3 |
| GND | GND |

The load's positive wire runs through the relay's COM and NO terminals. The load's supply never
touches the board.

## Run without hardware

```sh
sh tools/check.sh
.venv/bin/python examples/switch/rehearse.py
```

It prints on, on again, a refused value and off. `tests/switch_tests.cpp` covers the 30-second
safe-off. This is host evidence only.

## Build and first image

Follow [Ship a gadget](../../docs/ship-a-gadget.md) with `esp32s3-switch`.

## Hardware check

With the gadget Online and claimed, ask Grok Bot:

1. "Turn the switch on." Expect a click and the load on, and `switch.on: true`.
2. "Turn it on again." Expect no click and no change.
3. "Turn the switch off." Expect a click and the load off.
4. Turn it on, then unplug the home router (or the relay's network). Between 30 and 35 seconds
   later the load turns off. Once Online again, the state says `off_reason: link_lost`.
5. Power-cycle the board with the load on: it comes back off.
