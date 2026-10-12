#pragma once
#include "Arduino.h"
#include <map>
#include <string>
namespace fake {
extern std::map<int, std::string> screen; // Row to drawn text.
} // namespace fake
extern const uint8_t u8x8_font_chroma48medium8_r[];
constexpr int U8X8_PIN_NONE = 255;
struct FakeU8x8 {
  FakeU8x8(int = U8X8_PIN_NONE, int = U8X8_PIN_NONE, int = U8X8_PIN_NONE) {}
  void setI2CAddress(uint8_t) {}
  bool begin() { return true; }
  void setFont(const uint8_t *) {}
  void clearDisplay() { fake::screen.clear(); }
  void drawString(int, int row, const char *text) { fake::screen[row] = text; }
};
using U8X8_SSD1306_128X64_NONAME_HW_I2C = FakeU8x8;
using U8X8_SH1106_128X64_NONAME_HW_I2C = FakeU8x8;
