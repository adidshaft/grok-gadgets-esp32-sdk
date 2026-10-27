# Servo

An SG90 hobby servo on a Wi-Fi gadget. It points or opens something, such as a pet-feeder flap.
The design is hub
[ADR 013 §6](https://github.com/adidshaft/grok-gadgets/blob/main/docs/decisions/013-builder-kit.md).

**Not tested on hardware.** The sketch passes host tests and compiles for the board; nobody has
run it with the part yet.

| Name | Kind | What it does |
| --- | --- | --- |
| `servo` | State | `angle` from 0 to 180 |
| `servo.move` | Command | `{"angle": integer}` from 0 to 180, at about 180 degrees a second |
| `button` | Event | The BOOT button. Holding it 10 seconds is a factory reset |

The acknowledgement comes after the move ends, so Grok Bot's `gadgets_command_status` says when it
finished. Out-of-range or fractional angles are refused and the servo stays put. It starts at 90
degrees. The pulse runs from 500 µs (0°) to 2500 µs (180°); some servos buzz at the ends.

## Parts

- ESP32-S3-DevKitC-1 with an ESP32-S3-WROOM-1-N16R8 module.
- An SG90 9 g servo, the 180° kind (not "360° continuous rotation"). An MG90S also fits.
- A separate 5 V supply of at least 1 A: a USB charger with a USB-C breakout board that has
  5.1 kΩ CC resistors, or a breadboard power module set to 5 V.
- A 470 µF, 16 V capacitor across the servo's 5 V and ground (recommended).
- Jumper wires.

The board's own 3.3 V cannot power a servo, and a servo on the board's 5 V pin can reset the board.

## Wiring

Unplug USB and the 5 V supply first.

| Servo wire | Goes to |
| --- | --- |
| Orange or yellow (signal) | Board GPIO6 |
| Red (+5 V) | External supply +5 V |
| Brown or black (ground) | External supply ground |
| — | One wire from the external supply ground to a board GND |

The shared ground is required: without it the servo ignores the signal.

## Run without hardware

```sh
sh tools/check.sh
.venv/bin/python examples/servo/rehearse.py
```

It prints two moves and a refused angle. This is host evidence only.

## Build and first image

Follow [Ship a gadget](../../docs/ship-a-gadget.md) with `esp32s3-servo`.

## Hardware check

With the gadget Online and claimed, ask Grok Bot:

1. "Move the servo to 0 degrees." Expect the arm to sweep and stop, then a finished status and
   `servo.angle: 0`.
2. "Move it to 180." Expect a sweep of about a second, then the status.
3. "Move it to 200." Expect a refusal; the arm stays put.
4. Ask for 90 and watch the board: it must not reset during the move.
