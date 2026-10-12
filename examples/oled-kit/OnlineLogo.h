// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <stdint.h>

namespace oledkit {
// A pixel interpretation of the user-provided Grok Bot round face. The source reference is
// the hub's website/media/grok-bot-mark.svg. Only the rightmost 16x16 header pixels are drawn.
class OnlineLogo {
public:
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
    constexpr uint16_t face[] = {0x01e0, 0x07f8, 0x0ffc, 0x1ffe, 0x3fff, 0x3fff, 0x3fff,
                                 0x3fff, 0x3fff, 0x3fff, 0x1ffe, 0x0ffc, 0x07f8, 0x01e0};
    constexpr uint16_t eyes[] = {0, 0, 0x0660, 0x0330, 0x0330, 0x0180};
    constexpr int8_t bob[] = {0, 0, -1, -1, -1, 0, 0, 0, 1, 1, 1, 0};
    const int top = 1 + bob[frame_ % 12];
    const bool blink = frame_ == 20; // One 150 ms blink per 3.6 second cycle.
    uint8_t tiles[32] = {};
    for (int y = 0; y < 14; ++y) {
      const uint16_t cutout = blink ? (y == 4 ? 0x0770 : 0) : (y < 6 ? eyes[y] : 0);
      const uint16_t pixels = face[y] & ~cutout;
      for (int x = 0; x < 14; ++x)
        if (pixels & (1u << x)) {
          const int py = y + top;
          tiles[(py / 8) * 16 + x + 1] |= uint8_t(1u << (py % 8));
        }
    }
    // U8x8 tiles are eight vertical pixel columns. Writing the whole area removes old pixels.
    display.drawTile(14, 0, 2, tiles);
    display.drawTile(14, 1, 2, tiles + 16);
  }

private:
  static constexpr uint32_t FrameMs = 150;
  static constexpr uint8_t FrameCount = 24;
  uint32_t frameSince_ = 0;
  uint8_t frame_ = 0;
};
} // namespace oledkit
