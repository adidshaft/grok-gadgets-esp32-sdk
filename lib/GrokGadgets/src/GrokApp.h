// SPDX-License-Identifier: Apache-2.0
// A Wi-Fi gadget without a display, ready to ship (hub ADR 013): the relay link with the
// pinned CA (ADR 005, 009), the setup hotspot for phone setup (ADR 010), claim codes shown on
// the setup page (ADR 011 §10), signed updates when the build has a key (ADR 012), the USB
// console for factory provisioning, and BOOT held 10 seconds as a factory reset.
//
// Use it from one sketch file:
//
//   grok::Gadget gadget("Climate sensor", GROK_APP_VERSION, "climate");
//   grok::WifiApp app(gadget);
//   void setup() { /* register capabilities */ app.begin(); }
//   void loop() { app.loop(); }
//
// The status line goes to the USB console (`status: Online`). A sketch with a display can
// read `app.status()`.
#pragma once
#include "GrokSession.h"
#include "GrokSetup.h"
#include "GrokWifi.h"

#ifndef GROK_APP_VERSION
#define GROK_APP_VERSION "0.2.0-alpha.18"
#endif

#if defined(GROK_UPDATE_KEY_HEADER) && !defined(GROK_HOST_SIMULATION)
#define GROK_APP_UPDATES
#include "GrokUpdateEsp.h"
#include <grok_update_key.h>
// Arduino marks a new image valid at boot unless this returns true; the updater decides.
extern "C" bool verifyRollbackLater() { return true; }
#endif

namespace grok {
class WifiApp {
public:
  static constexpr uint32_t FactoryResetHoldMs = 10000, FactoryResetWarnMs = 3000;
  static constexpr size_t StatusSize = 24;

  explicit WifiApp(Gadget &gadget, int bootPin = 0)
      : gadget_(gadget), bootPin_(bootPin), link_(settings_),
        console_(Serial, store_, gadget.deviceId()), hotspot_(store_, settings_, factory_)
#ifdef GROK_APP_UPDATES
        ,
        crypto_(GrokUpdateKey), updater_(platform_, crypto_, GrokUpdateBoard, GROK_APP_VERSION)
#endif
  {
  }

  // After the capabilities are registered.
  void begin() {
    Serial.setRxBufferSize(MaxConsoleLine + 1024); // One provisioning line fits.
    Serial.begin(115200);
    pinMode(bootPin_, INPUT_PULLUP);
    store_.loadFactory(factory_);
    store_.load(settings_);
    link_.begin(gadget_.deviceId());
    console_.status(consoleStatus, this);
    gadget_.showClaimCodes(); // Ignored by relays without accounts.
#ifdef GROK_APP_UPDATES
    updater_.begin(millis());
    if (crypto_.keys())
      gadget_.updates(updater_);
#endif
    gadget_.begin(link_);
    hotspot_.begin(gadget_.deviceId(), millis()); // After begin: the device ID is set there.
  }

  void loop() {
    gadget_.loop();
    console_.loop();
    uint32_t now = millis();
#ifdef GROK_APP_UPDATES
    updater_.loop(now);
#endif
    hotspot_.loop(now, gadget_.connected(), gadget_.claimCode());
    if (hotspot_.takeJoined()) {
      link_.begin(gadget_.deviceId(), true); // Keep the hotspot so the page can show Online.
      gadget_.claimAfterSetup();
    }
    bool pressed = digitalRead(bootPin_) == LOW;
    if (pressed && !bootHeld_)
      bootHeldSince_ = now;
    bootHeld_ = pressed;
    uint32_t held = bootHeld_ ? uint32_t(now - bootHeldSince_) : 0;
    char status[StatusSize];
    if (console_.restartRequested())
      snprintf(status, sizeof(status), "%s", status_);
    else if (held >= FactoryResetHoldMs && console_.factoryReset())
      snprintf(status, sizeof(status), "Factory reset");
    else if (held >= FactoryResetWarnMs)
      snprintf(status, sizeof(status), "Reset in %us",
               unsigned((FactoryResetHoldMs - held + 999) / 1000));
    else
      describe(status);
    if (strcmp(status, status_)) {
      snprintf(status_, sizeof(status_), "%s", status);
      // The console never prints a claim code: the setup page and Grok Bot carry it.
      const char *said = strncmp(status_, "Code ", 5) ? status_ : "Code shown on the setup page";
      char line[64];
      size_t size = snprintf(line, sizeof(line), "status: %s\n", said);
      Serial.write(reinterpret_cast<const uint8_t *>(line), size < sizeof(line) ? size : 0);
    }
    if (console_.restartRequested()) {
      delay(300); // Let the console reply leave the USB buffer.
      ESP.restart();
    }
  }

  const char *status() const { return status_; }
#ifdef GROK_HOST_SIMULATION
  WifiLink &linkForTests() { return link_; } // Host tests script the relay on this link.
  SetupHotspot &hotspotForTests() { return hotspot_; }
#endif
  bool online() const { return gadget_.connected(); }
  bool provisioned() const { return settings_.complete(); }
  // How long the relay link has been down, 0 while Online. A switch reads this to turn its
  // load off after a link loss (hub ADR 013 §6).
  uint32_t offlineFor() const { return online_ ? 0 : uint32_t(millis() - offlineSince_); }

private:
  void describe(char (&row)[StatusSize]) {
    using State = SetupHotspot::State;
    if (gadget_.connected())
      online_ = true;
    else if (online_) {
      online_ = false;
      offlineSince_ = millis();
    }
#ifdef GROK_APP_UPDATES
    if (updater_.state() != Updater::State::Idle) {
      snprintf(row, sizeof(row), "Update %u%%", unsigned(updater_.percent()));
      return;
    }
    if (updater_.showReverted()) {
      snprintf(row, sizeof(row), "Reverted");
      return;
    }
#endif
    switch (hotspot_.state()) {
    case State::Open:
      snprintf(row, sizeof(row), "Setup %u min", unsigned(hotspot_.minutesLeft(millis())));
      return;
    case State::Joining:
      snprintf(row, sizeof(row), "Joining WiFi");
      return;
    default:
      break;
    }
    const char *text;
    if (!settings_.complete())
      text = "Not provisioned";
    else if (gadget_.connected() && gadget_.claimCode()) {
      snprintf(row, sizeof(row), "Code %s", gadget_.claimCode());
      return;
    } else if (gadget_.connected())
      text = "Online";
    else if (gadget_.rejected())
      text = "Token rejected";
    else if (link_.state() == WifiLink::State::WifiConnecting)
      text = grok::describe(link_.failure()).status;
    else if (link_.state() == WifiLink::State::TlsRejected)
      text = "TLS rejected";
    else if (link_.state() == WifiLink::State::NameUnresolved)
      text = "Relay name?";
    else
      text = "Relay...";
    snprintf(row, sizeof(row), "%s", text);
  }
  static void consoleStatus(JsonObject reply, void *raw) {
    auto &self = *static_cast<WifiApp *>(raw);
    reply["firmware"] = GROK_APP_VERSION;
    reply["provisioned"] = self.settings_.complete();
    reply["factory"] = self.factory_.complete(); // Never a value.
    reply["setup"] = SetupHotspot::stateName(self.hotspot_.state());
    reply["wifi"] = WiFi.status() == WL_CONNECTED ? "connected" : "disconnected";
    reply["claimed"] = self.gadget_.claimed(); // Never the code.
#ifdef GROK_APP_UPDATES
    reply["updates"] = self.crypto_.keys() ? "ready" : "no_key";
    reply["update_trial"] = self.updater_.onTrial();
#endif
    reply["gateway"] = strncmp(self.status_, "Code ", 5) ? self.status_ : "Online";
  }

  Gadget &gadget_;
  int bootPin_;
  WifiSettings settings_;
  FactorySettings factory_;
  NvsStore store_;
  WifiLink link_;
  Console console_;
  SetupHotspot hotspot_;
#ifdef GROK_APP_UPDATES
  EspUpdatePlatform platform_;
  MbedtlsUpdateCrypto crypto_;
  Updater updater_;
#endif
  char status_[StatusSize] = {};
  uint32_t bootHeldSince_ = 0, offlineSince_ = 0;
  bool bootHeld_ = false, online_ = false;
};
} // namespace grok
