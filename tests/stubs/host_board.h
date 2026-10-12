#pragma once
// Definitions for the simulated board; include once per host executable.
#undef NDEBUG // Tests are assertions.
#include "Arduino.h"
#include "ESPmDNS.h"
#include "Preferences.h"
#include "U8x8lib.h"
#include "WiFi.h"
#include "Wire.h"
namespace fake {
uint32_t now = 0;
int pin = 1;
uint32_t pixel = 0;
int shows = 0;
int led = 0;
uint32_t neopixel = 0;
uint8_t i2cAddress = 0x3c;
std::map<int, std::string> screen;
std::map<std::pair<int, int>, std::array<uint8_t, 8>> displayTiles;
int tileWrites = 0, textDraws = 0, displayClears = 0;
std::map<std::string, std::string> nvs;
bool nvsFails = false;
} // namespace fake
FakeWiFi WiFi;
FakeMDNS MDNS;
const uint8_t u8x8_font_chroma48medium8_r[] = {0};
FakeWire Wire;
FakeSerial Serial;
FakeEsp ESP;
