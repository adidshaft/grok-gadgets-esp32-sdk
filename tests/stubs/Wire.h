#pragma once
#include "Arduino.h"
namespace fake {
extern uint8_t i2cAddress; // The simulated display's 7-bit address; 0 for no display.
} // namespace fake
struct FakeWire {
  uint8_t target = 0;
  void begin(int, int) {}
  void end() {}
  void beginTransmission(uint8_t address) { target = address; }
  uint8_t endTransmission() { return fake::i2cAddress && target == fake::i2cAddress ? 0 : 2; }
};
extern FakeWire Wire;
