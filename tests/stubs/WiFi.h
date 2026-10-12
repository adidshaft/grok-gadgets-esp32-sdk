#pragma once
#include "Arduino.h"
#include <string>
constexpr int WL_CONNECTED = 3, WL_DISCONNECTED = 6, WIFI_STA = 1;
struct FakeWiFi {
  int state = WL_DISCONNECTED;
  std::string ssid, password;
  int begins = 0, disconnects = 0;
  void disconnect() { ++disconnects; }
  void mode(int) {}
  void setSleep(bool) {}
  void setAutoReconnect(bool) {}
  void begin(const char *name, const char *pass) {
    ssid = name;
    password = pass;
    ++begins;
  }
  int status() const { return state; }
};
extern FakeWiFi WiFi;
