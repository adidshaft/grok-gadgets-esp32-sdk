#pragma once
#include "Arduino.h"
#include <cstring>
#include <string>
// A scripted TLS client: `accept` decides whether connect() succeeds, `error` is lastError().
struct WiFiClientSecure : Stream {
  std::string input, output, ca, host, address; // address: set when connecting by IPAddress
  uint16_t port = 0;
  bool accept = true, open = false;
  int error = 0, connects = 0, stops = 0;
  void setCACert(const char *pem) { ca = pem; }
  void setTimeout(uint32_t) {}
  void setHandshakeTimeout(unsigned long) {}
  int connect(const char *name, uint16_t number) {
    ++connects;
    host = name;
    port = number;
    open = accept;
    return accept;
  }
  int connect(IPAddress ip, uint16_t number, const char *name, const char *rootCa, const char *,
              const char *) {
    ++connects;
    address = ip.toString();
    host = name; // The name the certificate must match.
    ca = rootCa;
    port = number;
    open = accept;
    return accept;
  }
  void stop() {
    if (open)
      ++stops;
    open = false;
    input.clear();
  }
  uint8_t connected() { return open; }
  int lastError(char *buffer, size_t size) {
    if (size)
      buffer[0] = 0;
    return accept ? 0 : error;
  }
  int available() override { return open ? int(input.size()) : 0; }
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
    if (!open)
      return 0;
    output.append(reinterpret_cast<const char *>(data), size);
    return size;
  }
};
