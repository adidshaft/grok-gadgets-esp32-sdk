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
} // namespace fake
struct Stream {
  virtual ~Stream() = default;
  virtual int available() = 0;
  virtual int read() = 0;
  virtual size_t write(uint8_t ch) = 0;
  virtual size_t write(const uint8_t *data, size_t size) = 0;
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
struct FakeEsp {
  uint64_t getEfuseMac() const { return 0x123456789abcULL; }
};
extern FakeEsp ESP;
