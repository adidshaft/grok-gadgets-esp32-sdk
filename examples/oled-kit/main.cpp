// SPDX-License-Identifier: Apache-2.0
// Ordered kit: ESP32-S3-N16R8 DevKitC-style board, 1.3-inch 128x64 I2C OLED on GPIO8 (SDA)
// and GPIO9 (SCL); the firmware also accepts the two data wires swapped, the onboard RGB pixel
// (`rgb.set`) and the BOOT button (GPIO0, read after boot). Commands: `display.text` shows up to 32
// printable ASCII characters; `display.clear` blanks the screen. The display state reports what the
// firmware last drew, not what anyone saw.
//
// Build with -DOLED_KIT_WIFI (environment esp32s3-oled-kit-wifi) for Wi-Fi: TLS to the gateway's
// LAN listener with the provisioned CA, settings in NVS, the USB serial port as a provisioning
// console, a status line on the top row, and BOOT held for 10 seconds as a factory reset.
#include <GrokSession.h>
#ifdef OLED_KIT_WIFI
#include <GrokWifi.h>
#endif
#include <U8x8lib.h>
#include <Wire.h>

#ifndef OLED_KIT_RGB_PIN
#define OLED_KIT_RGB_PIN 48 // Onboard WS2812 on most N16R8 DevKitC-style boards; 38 on some.
#endif
constexpr int DataPinA = 8, DataPinB = 9, ButtonPin = 0, RgbPin = OLED_KIT_RGB_PIN;
constexpr size_t MaxText = 32, Columns = 16;
#ifdef OLED_KIT_WIFI
constexpr const char *FirmwareVersion = "0.2.0-wifi";
#else
constexpr const char *FirmwareVersion = "0.1.0";
#endif

// The delivered 1.3-inch module is an SH1106 (an SSD1306 driver left a lit column at the
// right edge). Build with -DOLED_KIT_SSD1306 for SSD1306 modules.
#ifdef OLED_KIT_SSD1306
using Oled = U8X8_SSD1306_128X64_NONAME_HW_I2C;
#else
using Oled = U8X8_SH1106_128X64_NONAME_HW_I2C;
#endif
Oled *oled = nullptr;   // Created at boot for the pins on which the display answered.
uint8_t address = 0;    // 7-bit I2C address found at boot; 0 when no display answered.
int sda = -1, scl = -1; // Pins on which the display answered.
char text[MaxText + 1] = {};
grok::Rgb colour;
// Global or static only. Gadget is about 40 KB and overflows the 8 KB loop stack.
grok::Gadget gadget("ESP32-S3 OLED kit", FirmwareVersion, "oledkit");
#ifdef OLED_KIT_WIFI
constexpr uint32_t FactoryResetHoldMs = 10000;
grok::WifiSettings settings;
grok::NvsStore store;
grok::WifiLink wifiLink(settings);
grok::Console console(Serial, store, gadget.deviceId());
const char *shownStatus = nullptr;
uint32_t bootHeldSince = 0;
bool bootHeld = false;

const char *linkStatus() {
  if (!settings.complete())
    return "Not provisioned";
  if (gadget.connected())
    return "Online";
  if (gadget.rejected())
    return "Token rejected";
  switch (wifiLink.state()) {
  case grok::WifiLink::State::WifiConnecting:
    return "WiFi...";
  case grok::WifiLink::State::TlsRejected:
    return "TLS rejected";
  default:
    return "Gateway...";
  }
}
void drawStatus() {
  if (!oled)
    return;
  char row[Columns + 1];
  snprintf(row, sizeof(row), "%-16s", shownStatus ? shownStatus : "");
  oled->drawString(0, 0, row);
}
void consoleStatus(JsonObject reply, void *) {
  reply["firmware"] = FirmwareVersion;
  reply["provisioned"] = settings.complete();
  reply["wifi"] = WiFi.status() == WL_CONNECTED ? "connected" : "disconnected";
  reply["gateway"] = linkStatus();
}
#endif

bool probe(uint8_t candidate) {
  Wire.beginTransmission(candidate);
  return Wire.endTransmission() == 0;
}
void draw() {
  oled->clearDisplay();
  size_t length = strlen(text);
  for (size_t row = 0; row * Columns < length; ++row) {
    char line[Columns + 1] = {};
    strncpy(line, text + row * Columns, Columns);
    oled->drawString(0, row * 2 + 2, line);
  }
#ifdef OLED_KIT_WIFI
  drawStatus(); // The status row survives text changes and clear.
#endif
}
void reportState(JsonObject state, void *) {
  grok::writeRgb(state, colour);
  auto display = state.createNestedObject("display");
  display["ready"] = address != 0;
  display["text"] = text;
  if (address) {
    display["sda"] = sda;
    display["scl"] = scl;
    char hex[5];
    snprintf(hex, sizeof(hex), "0x%02x", address);
    display["address"] = hex;
  }
}
grok::Error showText(JsonObjectConst args, void *) {
  const char *value = args["text"] | static_cast<const char *>(nullptr);
  if (args.size() != 1 || !value || !*value || strlen(value) > MaxText)
    return {"invalid_arguments", "Expected {\"text\": 1-32 printable ASCII characters}"};
  for (const char *c = value; *c; ++c)
    if (*c < 0x20 || *c > 0x7e)
      return {"invalid_arguments", "Text must be printable ASCII"};
  if (!address)
    return {"display_unavailable", "No OLED answered on I2C at boot"};
  strcpy(text, value);
  draw();
  return {};
}
grok::Error clearText(JsonObjectConst args, void *) {
  if (args.size() != 0)
    return {"invalid_arguments", "Expected {}"};
  if (!address)
    return {"display_unavailable", "No OLED answered on I2C at boot"};
  text[0] = 0;
  draw();
  return {};
}
grok::Error setRgb(JsonObjectConst args, void *) {
  grok::Rgb next;
  auto error = grok::readRgb(args, next);
  if (error.code)
    return error;
  neopixelWrite(RgbPin, next.on ? next.r : 0, next.on ? next.g : 0, next.on ? next.b : 0);
  colour = next;
  return {};
}
void setup() {
  neopixelWrite(RgbPin, 0, 0, 0);
  // Try the documented wiring first, then the two data wires swapped.
  for (auto pins : {std::make_pair(DataPinA, DataPinB), std::make_pair(DataPinB, DataPinA)}) {
    Wire.begin(pins.first, pins.second);
    for (uint8_t candidate : {0x3c, 0x3d})
      if (probe(candidate)) {
        address = candidate;
        sda = pins.first;
        scl = pins.second;
        break;
      }
    if (address)
      break;
    Wire.end();
  }
  if (address) {
    oled = new Oled(U8X8_PIN_NONE); // Uses the Wire bus begun above; passing pins hangs begin().
    oled->setI2CAddress(address << 1);
    oled->begin();
    oled->setFont(u8x8_font_chroma48medium8_r);
    oled->clearDisplay();
  }
  gadget.state(reportState);
  gadget.command("rgb.set", setRgb);
  gadget.command(
      "display.text", showText, nullptr,
      R"({"type":"object","description":"Show up to 32 printable ASCII characters on )"
      R"(the OLED","properties":{"text":{"type":"string","minLength":1,"maxLength":32}},)"
      R"("required":["text"],"additionalProperties":false})");
  gadget.command("display.clear", clearText, nullptr,
                 R"({"type":"object","description":"Blank the OLED","properties":{},)"
                 R"("additionalProperties":false})");
  gadget.button(ButtonPin);
#ifdef OLED_KIT_WIFI
  Serial.setRxBufferSize(grok::MaxConsoleLine + 1024); // One provisioning line fits.
  Serial.begin(115200);
  store.load(settings);
  wifiLink.begin();
  console.status(consoleStatus, nullptr);
  gadget.begin(wifiLink);
#else
  gadget.begin();
#endif
}
#ifdef OLED_KIT_WIFI
void loop() {
  gadget.loop();
  console.loop();
  uint32_t now = millis();
  bool pressed = digitalRead(ButtonPin) == LOW;
  if (pressed && !bootHeld)
    bootHeldSince = now;
  bootHeld = pressed;
  if (bootHeld && uint32_t(now - bootHeldSince) >= FactoryResetHoldMs &&
      !console.restartRequested()) {
    console.factoryReset();
    shownStatus = "Factory reset";
    drawStatus();
  }
  if (console.restartRequested()) {
    delay(300); // Let the console reply leave the USB buffer.
    ESP.restart();
    return;
  }
  const char *status = linkStatus();
  if (status != shownStatus) {
    shownStatus = status;
    drawStatus();
    char line[40];
    size_t size = snprintf(line, sizeof(line), "status: %s\n", status);
    Serial.write(reinterpret_cast<const uint8_t *>(line), size);
  }
}
#else
void loop() { gadget.loop(); }
#endif
