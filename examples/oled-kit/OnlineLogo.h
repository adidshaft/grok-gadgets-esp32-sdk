// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <stdint.h>

namespace oledkit {
// A pixel interpretation of the user-provided Grok Bot round face. The source reference is
// the hub's website/media/grok-bot-mark.svg, rasterized to a 46-pixel disc with its two
// tilted eyes. It fills a 48x48 area: tile columns 10-15 of tile rows 0-5.
// Bit x of a row is pixel column x, left to right.
constexpr uint64_t OnlineFace[46] = {
    0x00000ffc0000, 0x00007fff8000, 0x0003fffff000, 0x0007fffff800, 0x001fff87fe00, 0x003fff87ff00,
    0x007fff83ff80, 0x00ffff83ffc0, 0x01ffffc1ffe0, 0x03ffffc1fff0, 0x03ff0fe0fff0, 0x07fe07e07ff8,
    0x0ffe07f07ffc, 0x0ffe03f0fffc, 0x0fff01f9fffc, 0x1fff01fffffe, 0x1fff80fffffe, 0x1fff80fffffe,
    0x3fffc07fffff, 0x3fffe07fffff, 0x3fffe0ffffff, 0x3ffff1ffffff, 0x3fffffffffff, 0x3fffffffffff,
    0x3fffffffffff, 0x3fffffffffff, 0x3fffffffffff, 0x3fffffffffff, 0x1ffffffffffe, 0x1ffffffffffe,
    0x1ffffffffffe, 0x0ffffffffffc, 0x0ffffffffffc, 0x0ffffffffffc, 0x07fffffffff8, 0x03fffffffff0,
    0x03fffffffff0, 0x01ffffffffe0, 0x00ffffffffc0, 0x007fffffff80, 0x003fffffff00, 0x001ffffffe00,
    0x0007fffff800, 0x0003fffff000, 0x00007fff8000, 0x00000ffc0000};
// The same face with half-closed eyes, for the blink.
constexpr uint64_t OnlineBlink[46] = {
    0x00000ffc0000, 0x00007fff8000, 0x0003fffff000, 0x0007fffff800, 0x001ffffffe00, 0x003fffffff00,
    0x007fffffff80, 0x00fffff3ffc0, 0x01ffffe1ffe0, 0x03ffffe1fff0, 0x03ffffe3fff0, 0x07fffffffff8,
    0x0ffffffffffc, 0x0ffffffffffc, 0x0fffe3fffffc, 0x1fff81fffffe, 0x1fff83fffffe, 0x1fffcffffffe,
    0x3fffffffffff, 0x3fffffffffff, 0x3fffffffffff, 0x3fffffffffff, 0x3fffffffffff, 0x3fffffffffff,
    0x3fffffffffff, 0x3fffffffffff, 0x3fffffffffff, 0x3fffffffffff, 0x1ffffffffffe, 0x1ffffffffffe,
    0x1ffffffffffe, 0x0ffffffffffc, 0x0ffffffffffc, 0x0ffffffffffc, 0x07fffffffff8, 0x03fffffffff0,
    0x03fffffffff0, 0x01ffffffffe0, 0x00ffffffffc0, 0x007fffffff80, 0x003fffffff00, 0x001ffffffe00,
    0x0007fffff800, 0x0003fffff000, 0x00007fff8000, 0x00000ffc0000};

class OnlineLogo {
public:
  static constexpr int Column = 10, Columns = 6, Rows = 6; // In 8x8 tiles.

  void reset(uint32_t now) {
    frame_ = 0;
    frameSince_ = now;
    drawn_ = -1; // A full redraw follows every reset.
  }

  // True when the next frame differs from the one on screen and should be drawn.
  bool advance(uint32_t now) {
    const uint32_t elapsed = uint32_t(now - frameSince_);
    if (elapsed < FrameMs)
      return false;
    // Skip missed frames in one step. Unsigned subtraction also handles millis() wraparound.
    const uint32_t steps = elapsed / FrameMs;
    frameSince_ += steps * FrameMs;
    frame_ = (frame_ + steps % FrameCount) % FrameCount;
    return look() != drawn_;
  }

  template <class Display> void draw(Display &display) {
    drawn_ = look();
    const int top = 1 + bob(frame_);
    const uint64_t *rows = blinking(frame_) ? OnlineBlink : OnlineFace;
    uint8_t tiles[Columns * 8] = {}; // One tile row at a time keeps the stack small.
    for (int row = 0; row < Rows; ++row) {
      for (int x = 0; x < Columns * 8; ++x)
        tiles[x] = 0;
      for (int bit = 0; bit < 8; ++bit) {
        const int y = row * 8 + bit - top;
        if (y < 0 || y >= Size)
          continue;
        for (int x = 0; x < Size; ++x)
          if (rows[y] >> x & 1)
            tiles[x + 1] |= uint8_t(1u << bit);
      }
      // U8x8 tiles are eight vertical pixel columns. Writing the whole area removes old pixels.
      display.drawTile(Column, row, Columns, tiles);
    }
  }

private:
  static constexpr int Size = 46;
  static constexpr uint32_t FrameMs = 150;
  static constexpr uint8_t FrameCount = 24;
  static int bob(uint8_t frame) {
    constexpr int8_t offsets[] = {0, 0, -1, -1, -1, 0, 0, 0, 1, 1, 1, 0};
    return offsets[frame % 12];
  }
  static bool blinking(uint8_t frame) { return frame == 20; } // 150 ms per 3.6 s cycle.
  int look() const { return (bob(frame_) + 1) * 2 + blinking(frame_); }

  uint32_t frameSince_ = 0;
  uint8_t frame_ = 0;
  int drawn_ = -1;
};
} // namespace oledkit
