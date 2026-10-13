# First experiment: ESP32-S3 and OLED

Start with simulation. Then repeat the owner's tested USB and Wi-Fi setup.
The owner completed acceptance on 12 October 2026. These results cover one kit, one Mac and one router.

## Parts that arrived

| Part | Tested item |
| --- | --- |
| Board | YD-ESP32-23 v1.3, ESP32-S3 N16R8; sold as an AYWHP DevKitC-style board |
| Display | 1.3-inch 128×64 SH1106, marked RG1.30 IIC V2.2; I²C address `0x3c` |
| Wiring | ApTechDeals 840-point breadboard and jumper wires |
| Cable | USB-C data cable, connected to the board's native **USB** port |
| Light and button | Onboard RGB pixel on GPIO48; BOOT button on GPIO0 |

The ordered display was 0.96 inch. The delivered display is different. Do not use the order description to select a driver.
Inspect your own board before wiring. Similar boards can have different pins or controllers.

## 1. Check the software

1. Run the host checks in the [README](../README.md#quickstart).
2. Run the gateway simulator rehearsal.
3. Run the [OLED software model](../examples/oled-kit/README.md#run-without-hardware).
4. Check text, clear, invalid input, button edges and recovery.

The model does not open a serial port or operate a display. Keep simulation results separate from physical observations.

## 2. Wire the display

Disconnect power first. Check the printed pin names on both parts.

| OLED pin | Tested board pin |
| --- | --- |
| VCC | 3V3 |
| GND | GND |
| SCK | GPIO9 |
| SDA | GPIO8 |

The onboard light and button need no external wiring on this kit.
Use an external LED, series resistor and button only if your board lacks suitable parts. Document their pins and polarity first.

## 3. Check USB operation

1. Follow [build and flash](../examples/oled-kit/README.md#build-and-flash). Use `esp32s3-oled-kit`, not the C124 profile.
2. If the first upload cannot connect, hold BOOT while you connect the native **USB** port.
3. Wait two seconds. Release BOOT, then upload again.
4. Start the gateway and USB bridge. Confirm the device identity.
5. Send `display.text`, then `display.clear`. Record what you see and compare it with reported state.
6. Test the RGB light and BOOT press/release events.
7. Test disconnect, reconnect and restart. Do not infer a physical effect from an acknowledgement.

Use the [USB acceptance record](verification/oled-kit-2026-10-12.md) to compare results.

## 4. Check Wi-Fi operation

1. Follow the [Wi-Fi build and provisioning steps](../examples/oled-kit/README.md#wi-fi-build).
2. Use a 2.4 GHz network. Keep the gateway running on the Mac.
3. Reserve the Mac's LAN address on the router. Its certificate must match that address.
4. Provision the board over USB. Keep the network password and device token out of Git and public reports.
5. Power the board from a USB charger. Check that the OLED shows **Online**.
6. Repeat text, clear, button, network recovery, gateway restart and power-cycle checks.
7. Compare results with the [Wi-Fi acceptance record](verification/oled-kit-wifi-2026-10-12.md).

The board is wireless. The gateway still needs a running computer. A button press does not wake Grok Bot.
The owner tested Grok Bot with a local `rehearse` command, then with native tool calls through the gateway's remote route ([record](https://github.com/adidshaft/grok-gadgets/blob/main/docs/verification/grok-remote-https-2026-10-13.md)).

The board stores credentials without flash encryption. Revoke its token and change the Wi-Fi password if you lose it.

## 5. Report your result

Copy this table into a test report. Do not include credentials or private network addresses.

| Item | Your result |
| --- | --- |
| OS, board revision and display marking | |
| SDK and gateway versions | |
| Build profile and firmware checksum | |
| Pin map and USB port | |
| Text, clear, light and button observations | |
| Disconnect, restart and recovery | |
| Wi-Fi status and power source | |
| Grok Bot route and independent gateway log evidence | |
| Failed, skipped or untested checks | |

A second person's test remains open. AtomS3 Lite C124 is an optional build-tested example; it has no physical acceptance record here.
