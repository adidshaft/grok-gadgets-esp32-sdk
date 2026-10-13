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
#include "OnlineLogo.h"
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
// Holding BOOT this long erases the Wi-Fi settings. From FactoryResetWarnMs the status row
// counts down, so a person waiting for feedback can let go in time.
constexpr uint32_t FactoryResetHoldMs = 10000, FactoryResetWarnMs = 3000;
grok::WifiSettings settings;
grok::NvsStore store;
grok::WifiLink wifiLink(settings);
grok::Console console(Serial, store, gadget.deviceId());
char shownStatus[Columns + 1] = {}; // What the status row shows now.
char shownHint[Columns + 1] = {};   // Row 1: what to check after a failed Wi-Fi join.
oledkit::OnlineLogo onlineLogo;
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
    return grok::describe(wifiLink.failure()).status; // "WiFi..." or why the join failed.
  case grok::WifiLink::State::TlsRejected:
    return "TLS rejected";
  case grok::WifiLink::State::NameUnresolved:
    // The gateway's .local name did not answer, or the relay's DNS name did not resolve.
    return wifiLink.relay() ? "Relay name?" : "Gateway name?";
  default:
    return wifiLink.relay() ? "Relay..." : "Gateway...";
  }
}
void drawStatus() {
  if (!oled)
    return;
  char row[Columns + 1];
  snprintf(row, sizeof(row), "%-16s", shownStatus);
  oled->drawString(0, 0, row);
  const bool online = !strcmp(shownStatus, "Online");
  snprintf(row, sizeof(row), "%-16s", online ? "Grok Bot" : shownHint);
  oled->drawString(0, 1, row);
  if (online)
    onlineLogo.draw(*oled); // Columns 10-15 of rows 0-5, over the padding above.
}
void consoleStatus(JsonObject reply, void *) {
  reply["firmware"] = FirmwareVersion;
  reply["provisioned"] = settings.complete();
  reply["wifi"] = WiFi.status() == WL_CONNECTED ? "connected" : "disconnected";
  reply["gateway"] = linkStatus();
  grok::JoinFailureText failure = grok::describe(wifiLink.failure());
  if (failure.code) {
    reply["wifi_failure"] = failure.code;
    reply["wifi_hint"] = failure.advice;
  }
}
#endif

bool probe(uint8_t candidate) {
  Wire.beginTransmission(candidate);
  return Wire.endTransmission() == 0;
}
// Split text into at most `count` lines of 16 columns. With `words`, break at spaces;
// return false if the text then needs more lines.
bool wrapText(bool words, int count, char lines[3][Columns + 1]) {
  const size_t length = strlen(text);
  size_t at = 0;
  for (int line = 0; line < 3; ++line) {
    memset(lines[line], 0, Columns + 1);
    if (line >= count)
      continue;
    size_t end = at + Columns < length ? at + Columns : length;
    if (words && end < length) {
      size_t space = end;
      while (space > at && text[space] != ' ')
        --space;
      if (space > at)
        end = space;
    }
    memcpy(lines[line], text + at, end - at);
    at = end;
    while (words && at < length && text[at] == ' ')
      ++at;
  }
  return at >= length;
}
void draw() {
  oled->clearDisplay();
  char lines[3][Columns + 1];
  int firstRow = 2, rowStep = 2;
#ifdef OLED_KIT_WIFI
  // While Online the face fills columns 10-15 of rows 0-5, so text uses rows 6 and 7.
  // Two full rows still hold the 32-character maximum.
  if (!strcmp(shownStatus, "Online")) {
    firstRow = 6;
    rowStep = 1;
    if (!wrapText(true, 2, lines))
      wrapText(false, 2, lines);
  } else
#endif
    wrapText(false, 3, lines);
  for (int row = 0; row < 3; ++row)
    if (lines[row][0])
      oled->drawString(0, firstRow + row * rowStep, lines[row]);
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
  wifiLink.begin(gadget.deviceId());
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
  uint32_t held = bootHeld ? uint32_t(now - bootHeldSince) : 0;
  char status[Columns + 1], hint[Columns + 1] = {};
  if (console.restartRequested())
    snprintf(status, sizeof(status), "%s", shownStatus); // Keep the last message.
  else if (held >= FactoryResetHoldMs && console.factoryReset())
    snprintf(status, sizeof(status), "Factory reset");
  else if (held >= FactoryResetWarnMs)
    snprintf(status, sizeof(status), "Reset in %cs", // 7 down to 1
             char('0' + (FactoryResetHoldMs - held + 999) / 1000));
  else {
    snprintf(status, sizeof(status), "%s", linkStatus());
    if (wifiLink.state() == grok::WifiLink::State::WifiConnecting)
      snprintf(hint, sizeof(hint), "%s", grok::describe(wifiLink.failure()).hint);
  }
  if (strcmp(status, shownStatus) || strcmp(hint, shownHint)) {
    const bool wasOnline = !strcmp(shownStatus, "Online");
    strcpy(shownStatus, status);
    strcpy(shownHint, hint);
    onlineLogo.reset(now);
    if (oled && wasOnline != !strcmp(shownStatus, "Online"))
      draw(); // The face appears or goes and the text moves: redraw once.
    else
      drawStatus();
    // The console line explains a failed join; it never names the network.
    const char *advice = hint[0] ? grok::describe(wifiLink.failure()).advice : nullptr;
    char line[160];
    size_t size = advice ? snprintf(line, sizeof(line), "status: %s. %s\n", status, advice)
                         : snprintf(line, sizeof(line), "status: %s\n", status);
    Serial.write(reinterpret_cast<const uint8_t *>(line), size);
  } else if (oled && !strcmp(shownStatus, "Online") && onlineLogo.advance(now)) {
    onlineLogo.draw(*oled); // No delay, full-screen clear, text redraw or console output.
  }
  if (console.restartRequested()) {
    delay(300); // Let the console reply and the last status leave the USB buffer.
    ESP.restart();
  }
}
#else
void loop() { gadget.loop(); }
#endif
