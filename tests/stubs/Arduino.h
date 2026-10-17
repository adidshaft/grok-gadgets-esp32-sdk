#pragma once
#include <algorithm>
#include <cstdio>
#include <deque>
#include <stdint.h>
#include <string>
using std::min;
constexpr int INPUT = 1, INPUT_PULLUP = 2, OUTPUT = 3, LOW = 0, HIGH = 1;
namespace fake {
extern uint32_t now;
extern int pin;
extern uint32_t pixel;
extern int shows;
extern int led;
extern uint32_t neopixel;
} // namespace fake
// The parts of Arduino's IPAddress the firmware uses.
struct IPAddress {
  uint32_t value = 0;
  IPAddress() = default;
  IPAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
      : value(a | uint32_t(b) << 8 | uint32_t(c) << 16 | uint32_t(d) << 24) {}
  operator uint32_t() const { return value; }
  std::string toString() const {
    return std::to_string(value & 255) + "." + std::to_string(value >> 8 & 255) + "." +
           std::to_string(value >> 16 & 255) + "." + std::to_string(value >> 24);
  }
};
struct Stream {
  virtual ~Stream() = default;
  virtual int available() = 0;
  virtual int read() = 0;
  virtual size_t write(uint8_t ch) = 0;
  virtual size_t write(const uint8_t *data, size_t size) = 0;
  virtual void flush() {}
};
struct FakeSerial : Stream {
  std::string input, output;
  size_t rxBuffer = 256;
  void begin(int) {}
  size_t setRxBufferSize(size_t size) { return rxBuffer = size; }
  int available() override { return input.size(); }
  int read() override {
    unsigned char ch = input[0];
    input.erase(0, 1);
    return ch;
  }
  size_t write(uint8_t ch) override {
    output.push_back(char(ch));
    return 1;
  }
  size_t write(const uint8_t *data, size_t size) override {
    output.append(reinterpret_cast<const char *>(data), size);
    return size;
  }
};
extern FakeSerial Serial;
inline uint32_t millis() { return fake::now; }
inline void delay(uint32_t ms) { fake::now += ms; }
inline int digitalRead(int) { return fake::pin; }
inline void digitalWrite(int, int level) { fake::led = level; }
inline void pinMode(int, int) {}
inline void delayMicroseconds(uint32_t) {}
// LEDC (servo PWM) as in Arduino-ESP32 2.0.17; the host keeps the last duty.
namespace fake {
inline uint32_t &ledcDuty() {
  static uint32_t duty = 0;
  return duty;
}
} // namespace fake
inline double ledcSetup(uint8_t, double freq, uint8_t) { return freq; }
inline void ledcAttachPin(uint8_t, uint8_t) {}
inline void ledcWrite(uint8_t, uint32_t duty) { fake::ledcDuty() = duty; }
inline void neopixelWrite(int, uint8_t r, uint8_t g, uint8_t b) {
  fake::neopixel = (uint32_t(r) << 16) | (uint32_t(g) << 8) | b;
}
struct FakeEsp {
  int restarts = 0;
  uint64_t getEfuseMac() const { return 0x123456789abcULL; }
  void restart() { ++restarts; }
  uint32_t getFreeHeap() const { return 200000; }
};
extern FakeEsp ESP;
