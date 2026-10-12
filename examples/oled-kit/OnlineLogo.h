// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <stdint.h>

namespace oledkit {
// A pixel interpretation of the user-provided Grok Bot round face. The source reference is
// the hub's website/media/grok-bot-mark.svg, rasterized to a 22-pixel disc with its two
// tilted eyes. It fills a 24x24 area: tile columns 13-15 of tile rows 0-2.
class OnlineLogo {
public:
  static constexpr int Column = 13, Columns = 3, Rows = 3; // In 8x8 tiles.

  void reset(uint32_t now) {
    frame_ = 0;
    frameSince_ = now;
  }

  bool advance(uint32_t now) {
    const uint32_t elapsed = uint32_t(now - frameSince_);
    if (elapsed < FrameMs)
      return false;
    // Skip missed frames in one step. Unsigned subtraction also handles millis() wraparound.
    const uint32_t steps = elapsed / FrameMs;
    frameSince_ += steps * FrameMs;
    frame_ = (frame_ + steps % FrameCount) % FrameCount;
    return true;
  }

  template <class Display> void draw(Display &display) const {
    // Bit x of a row is pixel column x, left to right.
    constexpr uint32_t face[Size] = {0x003f00, 0x01ffe0, 0x03f9f0, 0x07f8f8, 0x0ff8fc, 0x1f1c7e,
                                     0x1f0e7e, 0x1f8ffe, 0x3f87ff, 0x3fc7ff, 0x3fefff, 0x3fffff,
                                     0x3fffff, 0x3fffff, 0x1ffffe, 0x1ffffe, 0x1ffffe, 0x0ffffc,
                                     0x07fff8, 0x03fff0, 0x01ffe0, 0x003f00};
    // While blinking, the same disc with each eye reduced to a short line.
    constexpr uint32_t disc[Size] = {0x003f00, 0x01ffe0, 0x03fff0, 0x07fff8, 0x0ffffc, 0x1ffffe,
                                     0x1ffffe, 0x1ffffe, 0x3fffff, 0x3fffff, 0x3fffff, 0x3fffff,
                                     0x3fffff, 0x3fffff, 0x1ffffe, 0x1ffffe, 0x1ffffe, 0x0ffffc,
                                     0x07fff8, 0x03fff0, 0x01ffe0, 0x003f00};
    constexpr int8_t bob[] = {0, 0, -1, -1, -1, 0, 0, 0, 1, 1, 1, 0};
    const int top = 1 + bob[frame_ % 12];
    const bool blink = frame_ == 20; // One 150 ms blink per 3.6 second cycle.
    uint8_t tiles[Rows * Columns * 8] = {};
    for (int y = 0; y < Size; ++y) {
      uint32_t pixels = blink ? disc[y] : face[y];
      if (blink && y == 5)
        pixels &= ~uint32_t(0x000700);
      if (blink && y == 7)
        pixels &= ~uint32_t(0x00f000);
      for (int x = 0; x < Size; ++x)
        if (pixels & (1u << x)) {
          const int py = y + top;
          tiles[(py / 8) * Columns * 8 + x + 1] |= uint8_t(1u << (py % 8));
        }
    }
    // U8x8 tiles are eight vertical pixel columns. Writing the whole area removes old pixels.
    for (int row = 0; row < Rows; ++row)
      display.drawTile(Column, row, Columns, tiles + row * Columns * 8);
  }

private:
  static constexpr int Size = 22;
  static constexpr uint32_t FrameMs = 150;
  static constexpr uint8_t FrameCount = 24;
  uint32_t frameSince_ = 0;
  uint8_t frame_ = 0;
};
} // namespace oledkit
