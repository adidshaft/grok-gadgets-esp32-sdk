#pragma once
#include "Arduino.h"
#include <array>
#include <cassert>
#include <cstring>
#include <map>
#include <string>
namespace fake {
extern std::map<int, std::string> screen; // Row to drawn text.
extern std::map<std::pair<int, int>, std::array<uint8_t, 8>> displayTiles;
extern int tileWrites, textDraws, displayClears;
} // namespace fake
extern const uint8_t u8x8_font_chroma48medium8_r[];
constexpr int U8X8_PIN_NONE = 255;
struct FakeU8x8 {
  FakeU8x8(int = U8X8_PIN_NONE, int = U8X8_PIN_NONE, int = U8X8_PIN_NONE) {}
  void setI2CAddress(uint8_t) {}
  bool begin() { return true; }
  void setFont(const uint8_t *) {}
  void clearDisplay() {
    fake::screen.clear();
    fake::displayTiles.clear();
    ++fake::displayClears;
  }
  void drawString(int column, int row, const char *text) {
    fake::screen[row] = text;
    for (size_t i = 0; i < strlen(text); ++i)
      fake::displayTiles.erase({column + int(i), row});
    ++fake::textDraws;
  }
  void drawTile(int column, int row, int count, uint8_t *data) {
    assert(column >= 0 && column + count <= 16 && row >= 0 && row < 8);
    for (int i = 0; i < count; ++i) {
      auto &tile = fake::displayTiles[{column + i, row}];
      std::copy(data + i * 8, data + (i + 1) * 8, tile.begin());
    }
    ++fake::tileWrites;
  }
};
using U8X8_SSD1306_128X64_NONAME_HW_I2C = FakeU8x8;
using U8X8_SH1106_128X64_NONAME_HW_I2C = FakeU8x8;
