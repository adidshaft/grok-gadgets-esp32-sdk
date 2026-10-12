// SPDX-License-Identifier: Apache-2.0
// Wi-Fi transport (hub ADR 005): TLS to the gateway's LAN listener with a pinned local CA,
// settings in NVS, and a USB serial console for provisioning and factory reset.
// The console never prints a stored value. The Wi-Fi password and token stay in NVS.
#pragma once
#include "GrokSession.h"
#include <ESPmDNS.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

namespace grok {
constexpr size_t MaxCaPem = 3072, MaxConsoleLine = 4096;
constexpr uint32_t TlsConnectSeconds = 5, TlsHandshakeSeconds = 8;
// One mDNS lookup of the gateway's name; it blocks the loop, so keep it short.
constexpr uint32_t MdnsQueryMs = 1000;
// While offline, start a fresh join this often. The core's auto-reconnect can stop retrying
// after a long access-point outage (seen in the Phase 4 router-off test).
constexpr uint32_t WifiRejoinMs = 15000;
// mbedTLS: the server certificate did not verify against the pinned CA.
constexpr int TlsVerifyFailed = -0x2700;
constexpr const char *WifiNamespace = "grok-wifi";

// Why the station could not join, from the core's disconnect reason codes (ESP-IDF
// wifi_err_reason_t). None of these carries the network name or password.
enum class JoinFailure { None, NoNetwork, WrongPassword, Timeout };
inline JoinFailure classifyDisconnect(uint8_t reason) {
  switch (reason) {
  case 201: // NO_AP_FOUND: name not seen. Includes 5 GHz-only networks and a name typo.
    return JoinFailure::NoNetwork;
  case 15:  // 4WAY_HANDSHAKE_TIMEOUT: the usual result of a wrong WPA2 password.
  case 202: // AUTH_FAIL
  case 204: // HANDSHAKE_TIMEOUT
    return JoinFailure::WrongPassword;
  default:
    return JoinFailure::None;
  }
}
struct JoinFailureText {
  const char *code, *status, *hint, *advice; // status and hint fit one 16-column OLED row.
};
inline JoinFailureText describe(JoinFailure failure) {
  switch (failure) {
  case JoinFailure::NoNetwork:
    return {"no_network", "No network", "2.4GHz? Name?",
            "Network not found. Use a 2.4 GHz network and check the name's upper and lower case."};
  case JoinFailure::WrongPassword:
    return {"wrong_password", "Wrong password", "Check password",
            "The network rejected the password. Provision the board again."};
  case JoinFailure::Timeout:
    return {"join_timeout", "Join timeout", "Check router",
            "No answer from the network. Check that the router is on and in range."};
  default:
    return {nullptr, "WiFi...", "", nullptr};
  }
}

struct WifiSettings {
  char ssid[33] = {};
  char password[65] = {};
  char token[257] = {};
  char host[64] = {}; // IPv4 address or mDNS name such as my-mac.local (hub ADR 006).
  uint16_t port = 0;
  char ca[MaxCaPem + 1] = {};
  bool complete() const { return ssid[0] && token[0] && host[0] && port && ca[0]; }
  // Overwrite every secret-bearing field, for copies that are no longer needed.
  void wipe() {
    memset(ssid, 0, sizeof(ssid));
    memset(password, 0, sizeof(password));
    memset(token, 0, sizeof(token));
    memset(host, 0, sizeof(host));
    memset(ca, 0, sizeof(ca));
    port = 0;
  }
};

inline bool copyString(JsonVariantConst value, char *out, size_t capacity, size_t minimum) {
  if (!value.is<const char *>())
    return false;
  const char *text = value.as<const char *>();
  size_t length = strlen(text);
  if (length < minimum || length >= capacity)
    return false;
  memcpy(out, text, length + 1);
  return true;
}
inline bool validIpv4(const char *text) {
  int parts = 0;
  const char *p = text;
  while (*p) {
    int value = 0, digits = 0;
    while (*p >= '0' && *p <= '9') {
      value = value * 10 + (*p++ - '0');
      if (++digits > 3 || value > 255)
        return false;
    }
    if (!digits || ++parts > 4)
      return false;
    if (*p == '.' && parts < 4)
      ++p;
    else if (*p)
      return false;
  }
  return parts == 4;
}
// An mDNS name: lower-case DNS labels ending in .local, as `lan-cert` writes it.
inline bool validLocalName(const char *text) {
  static constexpr const char *Suffix = ".local";
  size_t length = strlen(text), suffix = strlen(Suffix);
  if (length <= suffix || strcmp(text + length - suffix, Suffix))
    return false;
  size_t label = 0;
  for (size_t i = 0; i < length - suffix + 1; ++i) {
    char c = i < length - suffix ? text[i] : '.';
    if (c == '.') {
      if (!label || text[i - 1] == '-' || label > 63)
        return false;
      label = 0;
    } else if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || (c == '-' && label)) {
      ++label;
    } else {
      return false;
    }
  }
  return true;
}
inline bool printableToken(const char *token) {
  for (const char *c = token; *c; ++c)
    if (*c <= 0x20 || *c > 0x7e)
      return false;
  return true;
}
// Validate a `provision` console command into `next`, which is wiped on any error. Error
// messages never contain a value. WifiSettings is about 3.4 KB: keep it off the 8 KB loop
// stack (a stack copy here crashed the board on every provision line).
inline Error readSettingsInto(JsonObjectConst in, WifiSettings &next) {
  static constexpr const char *Begin = "-----BEGIN CERTIFICATE-----";
  next.wipe();
  Error error = [&]() -> Error {
    if (in.size() != 7)
      return {"invalid_settings", "Expected type, ssid, password, token, host, port and ca"};
    if (!copyString(in["ssid"], next.ssid, sizeof(next.ssid), 1))
      return {"invalid_settings", "Wi-Fi name must be 1-32 bytes"};
    if (!copyString(in["password"], next.password, sizeof(next.password), 0) ||
        (next.password[0] && strlen(next.password) < 8))
      return {"invalid_settings", "Wi-Fi password must be empty or 8-64 bytes"};
    if (!copyString(in["token"], next.token, sizeof(next.token), 16) || !printableToken(next.token))
      return {"invalid_settings", "Token must be 16-256 printable characters"};
    if (!copyString(in["host"], next.host, sizeof(next.host), 7) ||
        !(validIpv4(next.host) || validLocalName(next.host)))
      return {"invalid_settings", "Host must be an IPv4 address or NAME.local"};
    if (!in["port"].is<int>() || in["port"].as<int>() < 1 || in["port"].as<int>() > 65535)
      return {"invalid_settings", "Port must be 1-65535"};
    next.port = in["port"].as<uint16_t>();
    if (!copyString(in["ca"], next.ca, sizeof(next.ca), 64) ||
        strncmp(next.ca, Begin, strlen(Begin)) || !strstr(next.ca, "-----END CERTIFICATE-----"))
      return {"invalid_settings", "CA must be one PEM certificate up to 3072 bytes"};
    return {};
  }();
  if (error.code)
    next.wipe();
  return error;
}

class SettingsStore {
public:
  virtual ~SettingsStore() = default;
  virtual bool load(WifiSettings &settings) = 0;
  virtual bool save(const WifiSettings &settings) = 0;
  virtual bool erase() = 0;
};

// NVS namespace `grok-wifi`. Flash encryption is off: whoever holds the board can read it.
class NvsStore : public SettingsStore {
public:
  bool load(WifiSettings &s) override {
    if (!prefs_.begin(WifiNamespace, true))
      return false;
    prefs_.getString("ssid", s.ssid, sizeof(s.ssid));
    prefs_.getString("pass", s.password, sizeof(s.password));
    prefs_.getString("token", s.token, sizeof(s.token));
    prefs_.getString("host", s.host, sizeof(s.host));
    s.port = prefs_.getUShort("port", 0);
    prefs_.getString("ca", s.ca, sizeof(s.ca));
    prefs_.end();
    return s.complete();
  }
  bool save(const WifiSettings &s) override {
    if (!prefs_.begin(WifiNamespace, false))
      return false;
    bool ok = prefs_.clear() && prefs_.putString("ssid", s.ssid) &&
              prefs_.putString("pass", s.password) == strlen(s.password) &&
              prefs_.putString("token", s.token) && prefs_.putString("host", s.host) &&
              prefs_.putUShort("port", s.port) && prefs_.putString("ca", s.ca);
    prefs_.end();
    return ok;
  }
  bool erase() override {
    if (!prefs_.begin(WifiNamespace, false))
      return false;
    bool ok = prefs_.clear();
    prefs_.end();
    return ok;
  }

private:
  Preferences prefs_;
};

// TLS to the gateway LAN listener. Trusts only the provisioned CA; never insecure mode.
class WifiLink : public Link {
public:
  enum class State {
    NotProvisioned,
    WifiConnecting,
    GatewayConnecting,
    TlsRejected,
    NameUnresolved
  };
  explicit WifiLink(const WifiSettings &settings) : s_(settings) {}
  // `mdnsName` is this board's own mDNS host name, used when the gateway is a .local name.
  void begin(const char *mdnsName = "grok-gadget") {
    mdnsName_ = mdnsName;
    if (!s_.complete())
      return;
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false); // Lower latency for 100 ms polling.
    WiFi.setAutoReconnect(true);
    // Runs on the Wi-Fi event task: store only the reason code.
    WiFi.onEvent(
        [this](arduino_event_id_t, arduino_event_info_t info) {
          lastReason_ = info.wifi_sta_disconnected.reason;
        },
        ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    WiFi.begin(s_.ssid, s_.password);
    joinedAt_ = millis();
    client_.setCACert(s_.ca);
    client_.setTimeout(TlsConnectSeconds);
    client_.setHandshakeTimeout(TlsHandshakeSeconds);
  }
  bool open() override {
    client_.stop();
    if (!s_.complete()) {
      state_ = State::NotProvisioned;
      return false;
    }
    if (WiFi.status() != WL_CONNECTED) {
      state_ = State::WifiConnecting;
      uint32_t now = millis();
      JoinFailure seen = classifyDisconnect(lastReason_);
      if (seen != JoinFailure::None)
        failure_ = seen;
      if (uint32_t(now - joinedAt_) >= WifiRejoinMs) {
        if (failure_ == JoinFailure::None)
          failure_ = JoinFailure::Timeout; // A whole attempt with no reason we recognise.
        lastReason_ = 0;
        WiFi.disconnect();
        WiFi.begin(s_.ssid, s_.password);
        joinedAt_ = now;
      }
      return false;
    }
    failure_ = JoinFailure::None;
    lastReason_ = 0;
    bool connected;
    if (validLocalName(s_.host)) {
      // Find the gateway's current address by name, then check its certificate by name.
      if (!mdnsStarted_)
        mdnsStarted_ = MDNS.begin(mdnsName_);
      char label[sizeof(s_.host)] = {};
      memcpy(label, s_.host, strlen(s_.host) - strlen(".local"));
      IPAddress address = MDNS.queryHost(label, MdnsQueryMs);
      if (!uint32_t(address)) {
        state_ = State::NameUnresolved;
        return false;
      }
      connected = client_.connect(address, s_.port, s_.host, s_.ca, nullptr, nullptr);
    } else {
      connected = client_.connect(s_.host, s_.port);
    }
    if (connected) {
      state_ = State::GatewayConnecting;
      return true;
    }
    char reason[64];
    state_ = client_.lastError(reason, sizeof(reason)) == TlsVerifyFailed
                 ? State::TlsRejected
                 : State::GatewayConnecting;
    return false;
  }
  bool alive() override { return WiFi.status() == WL_CONNECTED && client_.connected(); }
  void close() override { client_.stop(); }
  Stream &stream() override { return client_; }
  const char *token() const override { return s_.token; }
  // Why the last open() failed or what it is waiting for.
  State state() const {
    if (!s_.complete())
      return State::NotProvisioned;
    if (WiFi.status() != WL_CONNECTED)
      return State::WifiConnecting;
    return state_;
  }
  // The last reason the station could not join; None once Wi-Fi is up.
  JoinFailure failure() const {
    return WiFi.status() == WL_CONNECTED ? JoinFailure::None : failure_;
  }

private:
  const WifiSettings &s_;
  WiFiClientSecure client_;
  State state_ = State::NotProvisioned;
  JoinFailure failure_ = JoinFailure::None;
  const char *mdnsName_ = "grok-gadget";
  bool mdnsStarted_ = false;
  volatile uint8_t lastReason_ = 0;
  uint32_t joinedAt_ = 0;
};

// One-line JSON console on the USB serial port: `status`, `provision`, `factory_reset`.
class Console {
public:
  using Status = void (*)(JsonObject reply, void *context);
  Console(Stream &port, SettingsStore &store, const char *deviceId)
      : port_(port), store_(store), deviceId_(deviceId) {}
  void status(Status writer, void *context) {
    status_ = writer;
    context_ = context;
  }
  // True once a reply asked for a restart (after provisioning or a factory reset).
  bool restartRequested() const { return restart_; }
  void loop() {
    while (port_.available()) {
      char ch = char(port_.read());
      if (ch == '\r')
        continue;
      if (ch != '\n') {
        if (used_ < MaxConsoleLine)
          line_[used_++] = ch;
        else
          dropping_ = true;
        continue;
      }
      line_[used_] = 0;
      if (dropping_)
        fail("invalid_request", "Line exceeds 4096 bytes");
      else if (used_)
        handle();
      used_ = 0;
      dropping_ = false;
    }
  }
  bool factoryReset() {
    restart_ = store_.erase();
    return restart_;
  }

private:
  void handle() {
    // Zero-copy parse: strings stay in line_, which is wiped below.
    StaticJsonDocument<512> doc;
    if (deserializeJson(doc, line_, used_) || !doc.is<JsonObject>()) {
      wipe();
      return; // Not a console command; stay silent like the USB bridge.
    }
    const char *type = doc["type"] | "";
    if (!strcmp(type, "status")) {
      reply(true);
    } else if (!strcmp(type, "provision")) {
      WifiSettings &settings = pending_; // A member, not a 3.4 KB stack copy.
      Error error = readSettingsInto(doc.as<JsonObjectConst>(), settings);
      if (error.code)
        fail(error.code, error.message);
      else if (!store_.save(settings))
        fail("storage_failed", "Could not write NVS");
      else {
        restart_ = true;
        reply(false);
      }
      settings.wipe();
    } else if (!strcmp(type, "factory_reset")) {
      if (factoryReset())
        reply(false);
      else
        fail("storage_failed", "Could not erase NVS");
    } else {
      fail("unknown_command", "Use status, provision or factory_reset");
    }
    wipe();
  }
  void wipe() { memset(line_, 0, sizeof(line_)); }
  void reply(bool withStatus) {
    StaticJsonDocument<384> out;
    out["ok"] = true;
    out["device_id"] = deviceId_;
    if (restart_)
      out["restarting"] = true;
    if (withStatus && status_)
      status_(out.as<JsonObject>(), context_);
    send(out);
  }
  void fail(const char *code, const char *message) {
    StaticJsonDocument<256> out;
    out["ok"] = false;
    auto error = out.createNestedObject("error");
    error["code"] = code;
    error["message"] = message;
    send(out);
  }
  void send(JsonDocument &out) {
    char text[512];
    size_t size = serializeJson(out, text, sizeof(text) - 1);
    text[size++] = '\n';
    port_.write(reinterpret_cast<const uint8_t *>(text), size);
    port_.flush();
  }

  Stream &port_;
  SettingsStore &store_;
  const char *deviceId_;
  Status status_ = nullptr;
  void *context_ = nullptr;
  WifiSettings pending_; // Validated provisioning input; wiped after each command.
  char line_[MaxConsoleLine + 1] = {};
  size_t used_ = 0;
  bool dropping_ = false, restart_ = false;
};
} // namespace grok
