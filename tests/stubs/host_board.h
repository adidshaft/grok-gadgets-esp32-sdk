#pragma once
// Definitions for the simulated board; include once per host executable.
#undef NDEBUG // Tests are assertions.
#include "Arduino.h"
namespace fake {
uint32_t now = 0;
int pin = 1;
uint32_t pixel = 0;
int shows = 0;
int led = 0;
} // namespace fake
FakeSerial Serial;
FakeEsp ESP;
