#pragma once
#include "Arduino.h"
#include <functional>
#include <map>
#include <string>
#include <vector>
constexpr int WL_CONNECTED = 3, WL_DISCONNECTED = 6, WIFI_STA = 1, WIFI_AP = 2, WIFI_AP_STA = 3;
enum wifi_auth_mode_t { WIFI_AUTH_OPEN = 0, WIFI_AUTH_WPA2_PSK = 3 };
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
  int currentMode = 0;
  void mode(int value) { currentMode = value; }
  // Soft AP: the last name, passphrase and station limit; whether it is up.
  std::string apName, apPassphrase;
  int apChannel = 0, apMaxStations = 0;
  bool apUp = false;
  bool softAP(const char *name, const char *pass, int channel, int, int maxStations) {
    apName = name;
    apPassphrase = pass;
    apChannel = channel;
    apMaxStations = maxStations;
    return apUp = true;
  }
  bool softAPdisconnect(bool) {
    apUp = false;
    return true;
  }
  IPAddress softAPIP() const { return IPAddress(192, 168, 4, 1); }
  // Scan: `networks` is what the next scan finds; scanComplete() reports it once collected.
  struct Found {
    std::string name;
    bool secure;
  };
  std::vector<Found> networks;
  int scans = 0;
  bool scanDone = false;
  int16_t scanNetworks(bool) {
    ++scans;
    scanDone = true;
    return -1;
  }
  int16_t scanComplete() const { return scanDone ? int16_t(networks.size()) : int16_t(-2); }
  void scanDelete() { scanDone = false; }
  std::string SSID(uint8_t i) const { return networks[i].name; }
  wifi_auth_mode_t encryptionType(uint8_t i) const {
    return networks[i].secure ? WIFI_AUTH_WPA2_PSK : WIFI_AUTH_OPEN;
  }
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
  // DNS: `dns` maps a name to its address; an unknown name fails, as the core's lookup does.
  std::map<std::string, IPAddress> dns;
  int lookups = 0;
  int hostByName(const char *name, IPAddress &result) {
    ++lookups;
    auto found = dns.find(name);
    if (found == dns.end())
      return 0;
    result = found->second;
    return 1;
  }
  // Deliver a station disconnect event, as the core's event task would.
  void fail(uint8_t reason) {
    arduino_event_info_t info;
    info.wifi_sta_disconnected.reason = reason;
    if (disconnected)
      disconnected(ARDUINO_EVENT_WIFI_STA_DISCONNECTED, info);
  }
};
extern FakeWiFi WiFi;
