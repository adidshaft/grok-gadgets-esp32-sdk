#pragma once
#include <algorithm>
#include <cstdio>
#include <deque>
#include <stdint.h>
#include <string>
using std::min;
constexpr int INPUT_PULLUP = 2, LOW = 0;
namespace fake {
extern uint32_t now;
extern int pin;
extern uint32_t pixel;
extern int shows;
} // namespace fake
struct FakeSerial {
  std::string input, output;
  void begin(int) {}
  int available() const { return input.size(); }
  int read() {
    unsigned char ch = input[0];
    input.erase(0, 1);
    return ch;
  }
  size_t write(uint8_t ch) {
    output.push_back(char(ch));
    return 1;
  }
  size_t write(const uint8_t *data, size_t size) {
    output.append(reinterpret_cast<const char *>(data), size);
    return size;
  }
};
extern FakeSerial Serial;
inline uint32_t millis() { return fake::now; }
inline void delay(uint32_t ms) { fake::now += ms; }
inline int digitalRead(int) { return fake::pin; }
inline void pinMode(int, int) {}
struct FakeEsp {
  uint64_t getEfuseMac() const { return 0x123456789abcULL; }
};
extern FakeEsp ESP;
