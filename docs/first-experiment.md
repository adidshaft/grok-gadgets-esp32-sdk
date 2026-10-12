# First experiment: ESP32-S3 and OLED

Use this plan for the first physical experiment. Keep the simulator-first stage.
The owner's kit passed physical acceptance on 12 October 2026. See the
[verification record](verification/oled-kit-2026-10-12.md) for the inspected parts and wiring.

## Parts

| Part | Ordered item | Check after delivery |
| --- | --- | --- |
| Board | AYWHP ESP32-S3 DevKitC-style; ESP32-S3-WROOM-1-N16R8; USB-C | Module marking, board revision, headers, USB route, flash and PSRAM configuration |
| Wiring | ApTechDeals 840-point breadboard and jumper-wire set | Header fit, jumper type, breadboard rail continuity |
| Display | Robocraze 0.96-inch 128×64 OLED; advertised SPI/I²C, 4-pin | Controller, actual interface, pin order, voltage requirements and address |
| Cable | USB-C data cable required; not confirmed in this order | Data transfer and connector fit |
| Optional input/output | External LED, suitable series resistor and momentary button if built-in parts are unsuitable | Obtain separately if needed; these parts are not confirmed in the order |

Do not infer the OLED controller from its size. Do not infer an I²C address or
pin order from the four-pin advertisement. A DevKitC-style board can differ
from the reference board. Do not use the reset control as the demo button.

## Stage 1: software simulation

1. Run the SDK host checks in the README. These test existing sketches.
2. Run the gateway simulator rehearsal. This checks the existing transport path;
   it does not verify an OLED implementation.
3. Run [the OLED software model](../examples/oled-kit/README.md) and its tests.
   Check text, clear, invalid input, button edges and recovery without a driver.
4. Keep display state, transport acknowledgements and human observations separate.

The OLED model is isolated software, not a display driver or a gateway command.
Kit-specific firmware and end-to-end OLED gateway simulation remain pending. Existing RGB simulations remain
regression tests, not an OLED demonstration.

## Stage 2: inspect and wire

Disconnect power before wiring. Record photographs and readable part markings.
Confirm the supply requirements from the delivered module documentation.
Confirm whether the USB connector uses native USB or a USB-to-serial bridge.
Confirm the flash/PSRAM settings before creating the board build profile.

| Signal | Connection plan | Current status |
| --- | --- | --- |
| OLED power | Board supply compatible with the delivered OLED | Voltage and pin order provisional |
| OLED ground | Common board ground | Physical header pin provisional |
| Display data/clock | Selected board GPIOs appropriate to the confirmed interface | GPIOs and interface provisional |
| LED | Suitable built-in LED, or external LED with a series resistor | Type, polarity, pin and resistor provisional |
| Button | Suitable user button, or external momentary button with defined pull-up/down | GPIO, polarity and wiring provisional |

Do not reserve flash/PSRAM, USB or boot-strapping pins without checking the
board documentation. Record the final pin table before connection. If the OLED
interface differs from its advertisement, stop and revise the driver and wiring.
The existing 8 MB C124 build profile is not a verified N16R8 profile.

## Stage 3: physical acceptance

1. Compile the inspected board profile and selected display driver. Record source
   versions, settings and image checksums. Flash only the identified board.
2. Connect the board over USB to the local bridge and gateway. Verify discovery
   and the device identity from the running kit firmware.
3. Send a display text command. Observe the exact text on the OLED. Send clear.
   Observe an empty display. Compare both results with reported logical state.
4. If LED/button parts are present, observe LED on/off and ordered button press
   and release events. Otherwise record that subtest as blocked, not passed.
5. Disconnect during a command. Check offline status, pending/error reporting and
   reconnect. Reboot board and gateway. Check fresh state and session identity.
6. Repeat through the user's existing Grok Bot only after its connection is
   verified. Local simulation is not proof of Grok Bot operation.

Record the OS, USB port, board revision, driver, wiring, firmware hash, commands,
results and photographs. Keep credentials and local logs out of Git. No physical
acceptance is complete until someone observes the delivered hardware.

## Optional examples

The AtomS3 Lite C124 and generic LED/button examples remain SDK examples.
Their builds and prior test records remain useful. Neither provides a verified
pin map, OLED driver or flash profile for this kit.
