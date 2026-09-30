// SPDX-License-Identifier: Apache-2.0
#include "C124.h"
#include "GrokGadgets.h"
#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <esp_system.h>

Adafruit_NeoPixel pixel(grok::c124::PixelCount, grok::c124::RgbPin, NEO_GRB + NEO_KHZ800);
grok::Rgb colour;
grok::Button button;
void reportState(JsonObject state, void *) {
  grok::writeRgb(state, colour);
  state.createNestedObject("button")["pressed"] = button.pressed();
}
grok::Error setRgb(JsonObjectConst args, void *) {
  grok::Rgb next;
  auto error = grok::readRgb(args, next);
  if (error.code)
    return error;
  pixel.setPixelColor(0, next.on ? pixel.Color(next.r, next.g, next.b) : 0);
  pixel.show();
  colour = next;
  return {};
}
grok::Device gadget(reportState, nullptr);
DynamicJsonDocument outgoing(4096), incoming(4096), pendingAck(4096);
grok::Lines<> lines;
struct Edge {
  uint32_t sequence;
  bool pressed;
};
grok::Queue<Edge, 16> edges;
char bootId[32], deviceId[40];
uint32_t sequence = 0, lost = 0, reportedLost = 0, lastRequest = 0, lastState = 0, retryAt = 0,
         backoff = 500;
bool connected = false, waiting = false, haveAck = false;
enum class Kind { Hello, Poll, Ack, Event, State, Loss };
Kind pending = Kind::Hello;

void resetSession(uint32_t now) {
  // A possibly executed command cannot be acknowledged into a new gateway session.
  connected = false;
  waiting = false;
  haveAck = false;
  pendingAck.clear();
  lines.reset();
  retryAt = now + backoff;
  backoff = backoff < 5000 ? min(uint32_t(5000), backoff * 2) : 5000;
}
void transmit(Kind kind, uint32_t now) {
  if (outgoing.overflowed() || measureJson(outgoing) > grok::MaxFrame) {
    resetSession(now);
    return;
  }
  serializeJson(outgoing, Serial);
  Serial.write('\n');
  pending = kind;
  waiting = true;
  lastRequest = now;
}
void hello(uint32_t now) {
  outgoing.clear();
  outgoing["type"] = "hello";
  outgoing["protocol_version"] = grok::ProtocolVersion;
  auto d = outgoing.createNestedObject("device");
  d["device_id"] = deviceId;
  d["model"] = grok::c124::Model;
  d["firmware_version"] = "0.1.0";
  d["boot_id"] = bootId;
#ifdef GROK_HOST_SIMULATION
  d["simulated"] = true;
#else
  d["simulated"] = false;
#endif
  auto capabilities = d.createNestedArray("capabilities");
  gadget.capabilities(capabilities);
  capabilities.add("button");
  capabilities.add("state");
  capabilities.add("history_lost");
  gadget.state(d.createNestedObject("state"));
  transmit(Kind::Hello, now);
}
void receive(uint32_t now) {
  while (Serial.available()) {
    auto result = lines.feed(char(Serial.read()));
    if (result == decltype(lines)::Result::Overflow) {
      resetSession(now);
      return;
    }
    if (result != decltype(lines)::Result::Ready)
      continue;
    if (!waiting)
      continue; // Noise or delayed responses are never commands.
    incoming.clear();
    if (deserializeJson(incoming, lines.value()) || incoming.overflowed() ||
        !incoming["ok"].is<bool>()) {
      resetSession(now);
      return;
    }
    waiting = false;
    if (!incoming["ok"].as<bool>()) {
      // Errors preserve unconfirmed semantics. Event remains queued for next session.
      resetSession(now);
      return;
    }
    if (pending == Kind::Hello) {
      if (strcmp(incoming["protocol_version"] | "", grok::ProtocolVersion) ||
          !incoming["session_id"].is<const char *>()) {
        resetSession(now);
        return;
      }
      connected = true;
      backoff = 500;
    } else if (pending == Kind::Ack) {
      haveAck = false;
      pendingAck.clear();
    } else if (pending == Kind::Event) {
      edges.pop();
    } else if (pending == Kind::Loss) {
      lost -= reportedLost;
    } else if (pending == Kind::Poll) {
      if (!incoming["commands"].is<JsonArrayConst>() || incoming["commands"].size() > 1) {
        resetSession(now);
        return;
      }
      for (JsonObjectConst command : incoming["commands"].as<JsonArrayConst>()) {
        gadget.execute(command, pendingAck);
        haveAck = true;
      }
    }
  }
}
void setup() {
  Serial.begin(115200);
  pinMode(grok::c124::ButtonPin, INPUT_PULLUP);
  pixel.begin();
  pixel.clear();
  pixel.show();
  uint64_t mac = ESP.getEfuseMac();
  snprintf(deviceId, sizeof(deviceId), "c124-%04x%08x", unsigned(mac >> 32), unsigned(mac));
  snprintf(bootId, sizeof(bootId), "boot-%08x%08x", unsigned(esp_random()), unsigned(esp_random()));
  gadget.capability("rgb.set", setRgb, nullptr);
  retryAt = 500;
}
void loop() {
  uint32_t now = millis();
  if (button.sample(digitalRead(grok::c124::ButtonPin) == LOW, now)) {
    Edge edge{++sequence, button.pressed()};
    if (!edges.push(edge))
      ++lost;
  }
  receive(now);
  if (waiting) {
    if (uint32_t(now - lastRequest) > 3000)
      resetSession(now);
    delay(1);
    return;
  }
  if (!connected) {
    if (int32_t(now - retryAt) >= 0)
      hello(now);
    delay(1);
    return;
  }
  outgoing.clear();
  if (haveAck) {
    outgoing.set(pendingAck);
    transmit(Kind::Ack, now);
  } else if (edges.front()) {
    auto edge = *edges.front();
    char eventId[64];
    snprintf(eventId, sizeof(eventId), "%s-%lu", bootId, (unsigned long)edge.sequence);
    outgoing["type"] = "event";
    outgoing["event_id"] = eventId;
    outgoing["name"] = "button";
    outgoing.createNestedObject("data")["pressed"] = edge.pressed;
    transmit(Kind::Event, now);
  } else if (lost) {
    char eventId[64];
    snprintf(eventId, sizeof(eventId), "%s-%lu", bootId, (unsigned long)++sequence);
    outgoing["type"] = "event";
    outgoing["event_id"] = eventId;
    outgoing["name"] = "history_lost";
    reportedLost = lost;
    outgoing.createNestedObject("data")["dropped"] = reportedLost;
    transmit(Kind::Loss, now);
  } else if (uint32_t(now - lastState) >= 3000) {
    outgoing["type"] = "state";
    gadget.state(outgoing.createNestedObject("state"));
    lastState = now;
    transmit(Kind::State, now);
  } else if (uint32_t(now - lastRequest) >= 100) {
    outgoing["type"] = "poll";
    transmit(Kind::Poll, now);
  }
  delay(1);
}
