# Ordered OLED kit: demo specification

Target: AYWHP ESP32-S3 DevKitC-style ESP32-S3-WROOM-1-N16R8 USB-C board,
ApTechDeals 840-point breadboard/jumpers, and Robocraze 0.96-inch 128×64
advertised SPI/I²C 4-pin OLED. See [inspection and setup](../../docs/first-experiment.md).

This folder contains a runnable, controller-independent software model for the
next example. It contains no flashable sketch, OLED driver or gateway integration.
Do not copy the optional C124 pins or generic LED/button pins into this kit.

## Run without hardware

```sh
python3 examples/oled-kit/rehearse.py
python3 -m unittest discover -s examples/oled-kit -p 'test_*.py' -v
```

The model emits JSON with `simulated: true`. It uses no GPIO, USB or credentials.
It accepts up to 32 printable ASCII characters. This is a logical limit, not a
claim about the display font or physical layout. Button edges are injected
inputs. A reconnect retains model state; a reboot clears it and increments the
model boot counter. These choices do not predict the physical USB behavior.

## Logical behavior

| Action | Expected logical result | Physical check after delivery |
| --- | --- | --- |
| Set short text: `Grok Bot ready` | Display state contains the accepted text | OLED shows the same text |
| Clear | Display text is empty | OLED is blank |
| Invalid text type or unsupported length | Error; previous display state stays unchanged | Previous display remains unchanged |
| Reconnect or reboot | Read fresh state; do not claim the old text is still displayed | Observe the screen after recovery |
| Optional LED on/off | Report the commanded boolean state | Observe built-in or external LED |
| Optional button press/release | Retain ordered edge events | Operate the selected physical button |

Confirm the text length and character set again when implementing the renderer. Command names and schemas must match the
canonical gateway contract before adding a runnable simulator fixture. The model methods are not registered gateway commands. A successful software
acknowledgement does not prove that the physical screen changed.
