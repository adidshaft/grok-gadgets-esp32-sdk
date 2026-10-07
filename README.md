# Grok Gadgets ESP32 SDK

Build USB gadgets for your Grok Bot in an Arduino sketch: declare commands and events, and
`grok::Gadget` talks to the local [Grok Gadgets gateway](https://github.com/adidshaft/grok-gadgets-gateway).
Two examples compile; nobody has flashed either of them yet. Experimental alpha: see the
[project status](https://grok-gadgets.pages.dev/doc-docs-public-support-matrix).
Independent project, not affiliated with SpaceXAI or xAI.

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

Expected: four host test suites pass, both sketch transcripts pass the contract check, and the
C124 firmware compiles: **build verified, hardware pending**. Next,
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
[gateway](https://github.com/adidshaft/grok-gadgets-gateway). [Build, flash and recovery](docs/build-flash.md)
covers flash offsets, the device ID, `grok-gadgets-gateway enroll`, `usb-bridge` and unplug
recovery. Nobody has flashed a board for this project yet, so we welcome hardware results.

## Grok Bot today

Today you test the gateway locally. A cloud Grok Bot cannot reach
your computer yet, Wi-Fi is not built ([#5](https://github.com/adidshaft/grok-gadgets-esp32-sdk/issues/5)),
and a button press does not wake the Bot. See the
[hosting FAQ](https://github.com/adidshaft/grok-gadgets/blob/main/docs/getting-started/hosting.md).

## Learn more

| Topic | Guide |
| --- | --- |
| Commands, events and limits | [Library](docs/sdk.md) |
| Retries, loss reports and reconnects | [Protocol and recovery](docs/protocol-recovery.md) |
| C124 pins, and the GPIO48 `RGB_BUILTIN` trap | [Board sources](docs/board-sources.md) |
| What was checked, and what is still open | [Verification](docs/verification.md) and [build-checksums.json](docs/build-checksums.json) |
| Dependency licenses before you share firmware | [Dependencies](docs/dependencies.md) and [NOTICE](NOTICE) |

Host tests compile the sketches against a simulated board; they never open a serial port.

## Community

Show your build, ask for help and share ideas on
[r/GrokGadgets](https://www.reddit.com/r/GrokGadgets/). Report bugs through the
[issue chooser](https://github.com/adidshaft/grok-gadgets-esp32-sdk/issues/new/choose). New
here? Pick a [good first issue](https://github.com/adidshaft/grok-gadgets-esp32-sdk/issues?q=is%3Aissue+is%3Aopen+label%3A%22good+first+issue%22)
and read [CONTRIBUTING](CONTRIBUTING.md). Build errors: [SUPPORT](SUPPORT.md).
Vulnerabilities: [SECURITY](SECURITY.md).
Contribute on the `dev` branch; `main` holds tagged stable releases ([branches](CONTRIBUTING.md#branches)).

## License and affiliation

Apache-2.0; see [LICENSE](LICENSE) and [NOTICE](NOTICE). Grok Gadgets is an independent
open-source project. It is **not affiliated with, endorsed by or sponsored by SpaceXAI or
xAI**, which make Grok and Grok Bot, nor with M5Stack. We reconstructed the pre-publication commit dates; see the
[history record](https://github.com/adidshaft/grok-gadgets/blob/main/docs/verification/publication-sanitization.md).
