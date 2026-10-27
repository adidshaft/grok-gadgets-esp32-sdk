# Grok Gadgets ESP32 SDK

Build USB and Wi-Fi gadgets for your Grok Bot with an Arduino sketch. Declare commands and events with `grok::Gadget`.
The [gateway](https://github.com/adidshaft/grok-gadgets-gateway) connects your gadget to Grok Bot.
The owner tested one ESP32-S3 OLED kit over USB and Wi-Fi. Experimental alpha: see the
[project status](https://grokgadgets.org/doc-docs-public-support-matrix).
Independent project, not affiliated with SpaceXSI.

## Start here

| Path | Next step |
| --- | --- |
| [Use the tested kit](docs/first-experiment.md) | Wire and check the OLED kit |
| [Build a gadget](docs/sdk.md) | Declare commands, state and events |
| [Connect Grok Bot](https://github.com/adidshaft/grok-gadgets/blob/main/docs/getting-started/connect-grok-bot.md) | Use the current Grok Bot connection |

## First physical experiment

The delivered kit uses a **YD-ESP32-23 v1.3, ESP32-S3 N16R8** board and a **1.3-inch SH1106 OLED**.
It includes the ApTechDeals breadboard and jumper wires. The delivered display differs from the ordered 0.96-inch module.
Build the tested kit with the [first experiment](docs/first-experiment.md).
Without hardware, start with the host checks below.
The SDK supports custom gadgets. This kit is the tested example; AtomS3 Lite is an optional example with build evidence only.

> Writing style: ASD-STE100-inspired English. Use short sentences, active verbs and one instruction per step. This is not certified compliance.

## Quickstart

You need Python 3.11+, CMake 3.16+, Git and a C++14 compiler. No board or account.

```sh
git clone https://github.com/adidshaft/grok-gadgets-esp32-sdk.git
cd grok-gadgets-esp32-sdk
python3 -m venv .venv
.venv/bin/pip install -r requirements.lock
.venv/bin/pio pkg install
sh tools/check.sh
.venv/bin/python tools/check_contract.py
.venv/bin/pio run -e atoms3-lite-usb
```

Expected: nine host test suites pass, four sketch transcripts pass the contract check, and the
optional C124 firmware compiles: **build verified, hardware pending**. Next,
[create your own gadget](docs/sdk.md).

## What a gadget looks like

```cpp
#include <GrokSession.h>

bool on = false;
grok::Gadget gadget("Desk LED", "0.1.0", "esp32s3");

void report(JsonObject state, void *) { state["led"]["on"] = on; }
grok::Error setLed(JsonObjectConst args, void *) {
  on = args["on"];
  digitalWrite(4, on ? HIGH : LOW);  // your hardware
  return {};
}
void setup() {
  pinMode(4, OUTPUT);
  gadget.state(report);
  gadget.command("led.set", setLed, nullptr,
                 R"({"description":"Turn the desk LED on or off","type":"object",)"
                 R"("properties":{"on":{"type":"boolean"}},"required":["on"]})");
  gadget.button(0);
  gadget.begin();
}
void loop() { gadget.loop(); }
```

The schema's `description` tells the model what the command does. Declare the gadget as a
global (it uses about 40 KB). The full version is [`examples/led-button`](examples/led-button/main.cpp);
names, events and limits are in the [library guide](docs/sdk.md).

## Flash a board

The gadget talks over USB to a bridge on your computer, and the bridge talks to the
[gateway](https://github.com/adidshaft/grok-gadgets-gateway). The [first experiment](docs/first-experiment.md) describes the tested kit.
[Optional C124 build, flash and recovery](docs/build-flash.md)
covers flash offsets, the device ID, `grok-gadgets-gateway enroll`, `usb-bridge` and unplug
recovery for C124. Use the [OLED example](examples/oled-kit/README.md) for the tested ESP32-S3 kit. We welcome independent test results.

## Grok Bot today

The owner used Grok Bot to run `grok-gadgets-gateway rehearse` on the Mac.
The gateway log and the owner's display observation confirmed the Wi-Fi command ([record](docs/verification/oled-kit-wifi-2026-10-12.md#grok-bot)).
That was a local shell-command route. On 13 October 2026 Grok Bot also called the tools natively from the Mac and the iPhone. It showed text on this kit ([record](https://github.com/adidshaft/grok-gadgets/blob/main/docs/verification/grok-remote-https-2026-10-13.md), [how to connect](https://github.com/adidshaft/grok-gadgets/blob/main/docs/getting-started/connect-grok-bot.md)).
A USB charger powers the wireless board. An always-on relay can keep it available without the Mac.
The kit passed [relay checks](https://github.com/adidshaft/grok-gadgets/blob/main/docs/verification/mac-free-route-2026-10-13.md).
Phone setup passed on iPhone and Android on 14 October. Grok Bot calls from Android remain untested.
See the [phone setup record](https://github.com/adidshaft/grok-gadgets/blob/main/docs/verification/phone-setup-2026-10-14.md).
A button press does not wake Grok Bot. See the [roadmap](https://github.com/adidshaft/grok-gadgets/blob/main/ROADMAP.md).
The kit passed local-name, address-change and Mac login recovery checks. See
[always-on setup](https://github.com/adidshaft/grok-gadgets-gateway/blob/dev/docs/always-on.md).
A second builder has not yet reproduced the kit: [report your result](https://github.com/adidshaft/grok-gadgets/issues/185).

## Learn more

| Topic | Guide |
| --- | --- |
| Commands, events and limits | [Library](docs/sdk.md) |
| Retries, loss reports and reconnects | [Protocol and recovery](docs/protocol-recovery.md) |
| Optional C124 pins, and the GPIO48 `RGB_BUILTIN` trap | [Board sources](docs/board-sources.md) |
| What was checked, and what is still open | [Verification](docs/verification.md) and [build-checksums.json](docs/build-checksums.json) |
| Dependency licenses before you share firmware | [Dependencies](docs/dependencies.md) and [NOTICE](NOTICE) |

Host tests compile the sketches against a simulated board; they never open a serial port.

## Community

Show your build, ask for help and share ideas on
[Grok Gadgets community on X](https://x.com/i/communities/2026191471578067281). Report bugs through the
[issue chooser](https://github.com/adidshaft/grok-gadgets-esp32-sdk/issues/new/choose). New
here? Pick a [good first issue](https://github.com/adidshaft/grok-gadgets-esp32-sdk/issues?q=is%3Aissue+is%3Aopen+label%3A%22good+first+issue%22)
and read [CONTRIBUTING](CONTRIBUTING.md). Build errors: [SUPPORT](SUPPORT.md).
Vulnerabilities: [SECURITY](SECURITY.md).
Contribute on the `dev` branch; `main` holds tagged stable releases ([branches](CONTRIBUTING.md#branches)).

## License and affiliation

Apache-2.0; see [LICENSE](LICENSE) and [NOTICE](NOTICE). Grok Gadgets is an independent
open-source project. It is **not affiliated with, endorsed by or sponsored by SpaceXSI or
xAI**, which make Grok and Grok Bot, nor with M5Stack. We reconstructed the pre-publication commit dates; see the
[history record](https://github.com/adidshaft/grok-gadgets/blob/main/docs/verification/publication-sanitization.md).

## Project discussion

Ask questions and share gadget ideas in [hub Discussions](https://github.com/adidshaft/grok-gadgets/discussions).
Use this repository's issue forms for defects.
