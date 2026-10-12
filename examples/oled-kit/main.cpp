// SPDX-License-Identifier: Apache-2.0
// Ordered kit: ESP32-S3-N16R8 DevKitC-style board, 0.96-inch 128x64 I2C OLED on GPIO8 (SDA)
// and GPIO9 (SCL), the onboard RGB pixel (`rgb.set`) and the BOOT button (GPIO0, read after
// boot). Commands: `display.text` shows up to 32 printable ASCII characters; `display.clear`
// blanks the screen. The display state reports what the firmware last drew, not what anyone saw.
#include <GrokSession.h>
#include <U8x8lib.h>
#include <Wire.h>

#ifndef OLED_KIT_RGB_PIN
#define OLED_KIT_RGB_PIN 48 // Onboard WS2812 on most N16R8 DevKitC-style boards; 38 on some.
#endif
constexpr int SdaPin = 8, SclPin = 9, ButtonPin = 0, RgbPin = OLED_KIT_RGB_PIN;
constexpr size_t MaxText = 32, Columns = 16;

#ifdef OLED_KIT_SH1106
U8X8_SH1106_128X64_NONAME_HW_I2C oled;
#else
U8X8_SSD1306_128X64_NONAME_HW_I2C oled;
#endif
uint8_t address = 0; // 7-bit I2C address found at boot; 0 when no display answered.
char text[MaxText + 1] = {};
grok::Rgb colour;
// Global or static only. Gadget is about 40 KB and overflows the 8 KB loop stack.
grok::Gadget gadget("ESP32-S3 OLED kit", "0.1.0", "oledkit");

bool probe(uint8_t candidate) {
  Wire.beginTransmission(candidate);
  return Wire.endTransmission() == 0;
}
void draw() {
  oled.clearDisplay();
  size_t length = strlen(text);
  for (size_t row = 0; row * Columns < length; ++row) {
    char line[Columns + 1] = {};
    strncpy(line, text + row * Columns, Columns);
    oled.drawString(0, row * 2 + 2, line);
  }
}
void reportState(JsonObject state, void *) {
  grok::writeRgb(state, colour);
  auto display = state.createNestedObject("display");
  display["ready"] = address != 0;
  display["text"] = text;
  if (address) {
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
  oled.clearDisplay();
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
  Wire.begin(SdaPin, SclPin);
  for (uint8_t candidate : {0x3c, 0x3d})
    if (probe(candidate)) {
      address = candidate;
      break;
    }
  if (address) {
    oled.setI2CAddress(address << 1);
    oled.begin();
    oled.setFont(u8x8_font_chroma48medium8_r);
    oled.clearDisplay();
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
  gadget.begin();
}
void loop() { gadget.loop(); }
