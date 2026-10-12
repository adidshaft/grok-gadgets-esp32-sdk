#pragma once
#include "Arduino.h"
#include <functional>
#include <string>
constexpr int WL_CONNECTED = 3, WL_DISCONNECTED = 6, WIFI_STA = 1;
enum arduino_event_id_t { ARDUINO_EVENT_WIFI_STA_DISCONNECTED = 5 };
struct arduino_event_info_t {
  struct {
    uint8_t reason = 0;
  } wifi_sta_disconnected;
};
struct FakeWiFi {
  int state = WL_DISCONNECTED;
  std::string ssid, password;
  int begins = 0, disconnects = 0;
  std::function<void(arduino_event_id_t, arduino_event_info_t)> disconnected;
  void disconnect() { ++disconnects; }
  void mode(int) {}
  void setSleep(bool) {}
  void setAutoReconnect(bool) {}
  void onEvent(std::function<void(arduino_event_id_t, arduino_event_info_t)> callback,
               arduino_event_id_t) {
    disconnected = callback;
  }
  void begin(const char *name, const char *pass) {
    ssid = name;
    password = pass;
    ++begins;
  }
  int status() const { return state; }
  // Deliver a station disconnect event, as the core's event task would.
  void fail(uint8_t reason) {
    arduino_event_info_t info;
    info.wifi_sta_disconnected.reason = reason;
    if (disconnected)
      disconnected(ARDUINO_EVENT_WIFI_STA_DISCONNECTED, info);
  }
};
extern FakeWiFi WiFi;
