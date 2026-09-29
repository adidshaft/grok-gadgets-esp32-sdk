#pragma once
#include "Arduino.h"
constexpr int NEO_GRB = 1, NEO_KHZ800 = 2;
struct Adafruit_NeoPixel {
  Adafruit_NeoPixel(int, int, int) {}
  void begin() {}
  void clear() { fake::pixel = 0; }
  void show() { ++fake::shows; }
  uint32_t Color(uint8_t r, uint8_t g, uint8_t b) {
    return (uint32_t(r) << 16) | (uint32_t(g) << 8) | b;
  }
  void setPixelColor(int, uint32_t colour) { fake::pixel = colour; }
};
