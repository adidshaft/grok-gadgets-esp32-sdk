// SPDX-License-Identifier: Apache-2.0
// Arduino-ESP32 serial session: hello, polling, ACKs, events, loss reports and recovery.
#pragma once
#include "GrokGadgets.h"
#include <Arduino.h>
#include <esp_system.h>

#ifndef GROK_EVENT_QUEUE
#define GROK_EVENT_QUEUE 16
#endif
#ifndef GROK_EVENT_DATA
#define GROK_EVENT_DATA 128
#endif
#ifndef GROK_DOCUMENT_CAPACITY
#define GROK_DOCUMENT_CAPACITY 4096
#endif

namespace grok {
// Every JSON value needs at least two bytes of compact input, so any legal frame fits.
constexpr size_t IncomingCapacity = JSON_ARRAY_SIZE(MaxFrame / 2 + 8) + MaxFrame + 1;
// Longer than the USB bridge's worst case (2 s connect + 10 s reply), so a reply is never
// attributed to the next request. Protocol 0.1.0 has no request ID.
constexpr uint32_t RequestTimeoutMs = 13000;
constexpr uint32_t PollIntervalMs = 100, StateIntervalMs = 3000, MaxBackoffMs = 5000;
constexpr uint8_t MaxEventAttempts = 5;
constexpr size_t SerialRxBuffer = 2 * (MaxFrame + 1);

// One gadget per firmware. Declare it as a global or static object: it holds about 40 KB
// (retained ACK cache, line buffer and event queue) and would overflow the 8 KB loop stack.
class Gadget {
public:
  Gadget(const char *model, const char *firmwareVersion, const char *idPrefix = "esp32")
      : device_(writeState, this), model_(model), firmware_(firmwareVersion), prefix_(idPrefix),
        incoming_(IncomingCapacity), outgoing_(GROK_DOCUMENT_CAPACITY),
        ack_(GROK_DOCUMENT_CAPACITY) {}

  // Registration happens before begin(). Strings must outlive the gadget (use literals).
  // `schema` is an inline JSON Schema object for the arguments; Grok sees it as the contract.
  bool command(const char *name, Handler handler, void *context = nullptr,
               const char *schema = nullptr) {
    if (began_ || !room() || declared(name) ||
        (schema && (!strcmp(name, "rgb.set") || !validSchema(schema))))
      return false;
    if (!device_.capability(name, handler, context))
      return false;
    if (schema)
      schemas_[schemaCount_++] = {name, schema, false};
    return true;
  }
  // A custom event. `dataSchema` optionally describes the event data object.
  bool event(const char *name, const char *dataSchema = nullptr) {
    if (began_ || !room() || !validId(name) || reservedName(name) || declared(name) ||
        (dataSchema && !validSchema(dataSchema)))
      return false;
    eventNames_[eventCount_++] = name;
    schemas_[schemaCount_++] = {name, dataSchema, true};
    return true;
  }
  // Built-in debounced `button` event plus `state.button.pressed`.
  bool button(int pin, bool activeLow = true) {
    if (began_ || buttonPin_ >= 0 || !room())
      return false;
    buttonPin_ = pin;
    activeLow_ = activeLow;
    return true;
  }
  void state(StateWriter writer, void *context = nullptr) {
    writer_ = writer;
    writerContext_ = context;
  }

  // Queue a declared custom event. False if undeclared, too large or the queue is full;
  // a full queue is counted and reported later as `history_lost`.
  bool emit(const char *name, JsonObjectConst data) {
    Pending item;
    item.name = eventName(name);
    if (!item.name)
      return false;
    if (!data.isNull()) {
      if (data.size() > 32 || measureJson(data) >= sizeof(item.data))
        return false;
      serializeJson(data, item.data, sizeof(item.data));
    }
    return enqueue(item);
  }
  bool emit(const char *name) { return emit(name, JsonObjectConst()); }

  // USB CDC (HWCDC) defaults to a 256-byte RX queue and drops the rest.
  // Set the larger queue before begin(). Unverified on hardware.
  bool begin() {
    Serial.setRxBufferSize(SerialRxBuffer);
    Serial.begin(115200);
    return begin(Serial);
  }
  // Custom transports: configure the stream (and its RX buffer) before calling.
  bool begin(Stream &port) {
    port_ = &port;
    uint64_t mac = ESP.getEfuseMac(); // First MAC byte in the lowest byte.
    snprintf(deviceId_, sizeof(deviceId_), "%s-%02x%02x%02x%02x%02x%02x", prefix_,
             unsigned(mac & 0xff), unsigned(mac >> 8 & 0xff), unsigned(mac >> 16 & 0xff),
             unsigned(mac >> 24 & 0xff), unsigned(mac >> 32 & 0xff), unsigned(mac >> 40 & 0xff));
    snprintf(bootId_, sizeof(bootId_), "boot-%08x%08x", unsigned(esp_random()),
             unsigned(esp_random()));
    if (buttonPin_ >= 0)
      pinMode(buttonPin_, activeLow_ ? INPUT_PULLUP : INPUT);
    began_ = true;
    ready_ = validId(deviceId_) && buildHello();
    outgoing_.clear();
    retryAt_ = millis() + 500;
    return ready_;
  }

  // Call from loop(). Sleeps 1 ms per call; the RX buffer holds replies meanwhile.
  void loop() {
    if (!ready_) {
      delay(1);
      return;
    }
    uint32_t now = millis();
    sampleButton(now);
    receive(now);
    if (waiting_) {
      if (uint32_t(now - lastRequest_) > RequestTimeoutMs)
        resetSession(now);
    } else if (!connected_) {
      if (int32_t(now - retryAt_) >= 0)
        hello(now);
    } else {
      sendNext(now);
    }
    delay(1);
  }

  bool ready() const { return ready_; }
  bool connected() const { return connected_; }
  bool waiting() const { return waiting_; }
  bool pendingAck() const { return haveAck_; }
  size_t queued() const { return queue_.size(); }
  uint32_t lost() const { return lost_; }
  const char *deviceId() const { return deviceId_; }
  const char *bootId() const { return bootId_; }

private:
  enum class Kind { Hello, Poll, Ack, Event, State, Loss };
  struct Pending {
    uint32_t sequence = 0;
    const char *name = nullptr;
    char data[GROK_EVENT_DATA] = "{}";
  };
  struct Schema {
    const char *name, *schema;
    bool event;
  };
  struct Span {
    const char *p, *end;
    int read() { return p < end ? static_cast<unsigned char>(*p++) : -1; }
    size_t readBytes(char *buffer, size_t length) {
      size_t n = 0;
      while (n < length && p < end)
        buffer[n++] = *p++;
      return n;
    }
    bool blank() const {
      for (const char *c = p; c < end; ++c)
        if (*c != ' ' && *c != '\t' && *c != '\r')
          return false;
      return true;
    }
  };

  size_t names() const {
    return device_.size() + eventCount_ + (buttonPin_ >= 0 ? 1 : 0) + 2; // state, history_lost
  }
  bool room() const { return names() < MaxCapabilities; }
  bool declared(const char *name) const { return !name || device_.has(name) || eventName(name); }
  const char *eventName(const char *name) const {
    if (!name)
      return nullptr;
    for (size_t i = 0; i < eventCount_; ++i)
      if (!strcmp(eventNames_[i], name))
        return eventNames_[i];
    return nullptr;
  }
  bool validSchema(const char *schema) {
    Span in{schema, schema + strlen(schema)};
    incoming_.clear();
    return !deserializeJson(incoming_, in) && incoming_.is<JsonObject>() && in.blank();
  }
  static void writeState(JsonObject state, void *raw) {
    auto &self = *static_cast<Gadget *>(raw);
    if (self.writer_)
      self.writer_(state, self.writerContext_);
    if (self.buttonPin_ >= 0)
      state.createNestedObject("button")["pressed"] = self.button_.pressed();
  }
  bool enqueue(Pending &item) {
    item.sequence = ++sequence_;
    if (queue_.push(item))
      return true;
    ++lost_;
    return false;
  }
  void sampleButton(uint32_t now) {
    if (buttonPin_ < 0)
      return;
    bool level = digitalRead(buttonPin_) == HIGH;
    if (!button_.sample(level != activeLow_, now))
      return;
    Pending item;
    item.name = "button";
    snprintf(item.data, sizeof(item.data), "{\"pressed\":%s}",
             button_.pressed() ? "true" : "false");
    enqueue(item);
  }

  bool buildHello() {
    outgoing_.clear();
    outgoing_["type"] = "hello";
    outgoing_["protocol_version"] = ProtocolVersion;
    auto d = outgoing_.createNestedObject("device");
    d["device_id"] = deviceId_;
    d["model"] = model_;
    d["firmware_version"] = firmware_;
    d["boot_id"] = bootId_;
#ifdef GROK_HOST_SIMULATION
    d["simulated"] = true;
#else
    d["simulated"] = false;
#endif
    auto capabilities = d.createNestedArray("capabilities");
    device_.capabilities(capabilities);
    for (size_t i = 0; i < eventCount_; ++i)
      capabilities.add(eventNames_[i]);
    if (buttonPin_ >= 0)
      capabilities.add("button");
    capabilities.add("state");
    capabilities.add("history_lost");
    if (schemaCount_) {
      auto schemas = d.createNestedObject("capability_schemas");
      for (size_t i = 0; i < schemaCount_; ++i) {
        auto target = schemas.createNestedObject(schemas_[i].name);
        if (schemas_[i].schema) {
          if (!validSchema(schemas_[i].schema))
            return false;
          for (JsonPairConst member : incoming_.as<JsonObjectConst>())
            target[member.key()] = member.value();
        } else {
          target["type"] = "object";
        }
        // Gateway convention: an annotated capability is an event, never a callable command.
        if (schemas_[i].event)
          target["x-grok-gadgets-kind"] = "event";
      }
    }
    device_.state(d.createNestedObject("state"));
    return !outgoing_.overflowed() && measureJson(outgoing_) <= MaxFrame;
  }
  void hello(uint32_t now) {
    if (buildHello())
      transmit(Kind::Hello, now);
    else
      resetSession(now);
  }
  JsonObject startEvent(const char *name, uint32_t sequence) {
    char eventId[64];
    snprintf(eventId, sizeof(eventId), "%s-%lu", bootId_, static_cast<unsigned long>(sequence));
    outgoing_["type"] = "event";
    outgoing_["event_id"] = eventId;
    outgoing_["name"] = name;
    return outgoing_.as<JsonObject>();
  }
  void sendNext(uint32_t now) {
    outgoing_.clear();
    if (haveAck_) {
      outgoing_.set(ack_);
      transmit(Kind::Ack, now);
    } else if (auto *item = queue_.front()) {
      startEvent(item->name, item->sequence)["data"] =
          serialized(static_cast<const char *>(item->data));
      transmit(Kind::Event, now);
    } else if (lost_) {
      // One ID per report, reused on retry, so the gateway counts each loss once.
      if (!lossSequence_) {
        lossSequence_ = ++sequence_;
        reportedLost_ = lost_;
      }
      startEvent("history_lost", lossSequence_).createNestedObject("data")["dropped"] =
          reportedLost_;
      transmit(Kind::Loss, now);
    } else if (uint32_t(now - lastState_) >= StateIntervalMs) {
      outgoing_["type"] = "state";
      device_.state(outgoing_.createNestedObject("state"));
      lastState_ = now;
      transmit(Kind::State, now);
    } else if (uint32_t(now - lastRequest_) >= PollIntervalMs) {
      outgoing_["type"] = "poll";
      transmit(Kind::Poll, now);
    }
  }
  void transmit(Kind kind, uint32_t now) {
    if (outgoing_.overflowed() || measureJson(outgoing_) > MaxFrame) {
      resetSession(now);
      return;
    }
    serializeJson(outgoing_, *port_);
    port_->write('\n');
    pending_ = kind;
    waiting_ = true;
    lastRequest_ = now;
  }

  void receive(uint32_t now) {
    while (port_->available()) {
      auto result = lines_.feed(char(port_->read()));
      if (result == Lines<>::Result::Overflow && waiting_) {
        resetSession(now);
        return;
      }
      if (result != Lines<>::Result::Ready || !waiting_)
        continue; // Noise or unrequested lines are never commands.
      waiting_ = false;
      if (!reply(now))
        return;
    }
  }
  // Exactly one JSON object per line: no NUL, no trailing bytes except whitespace.
  DeserializationError parse() {
    Span in{lines_.value(), lines_.value() + lines_.size()};
    if (strlen(lines_.value()) != lines_.size())
      return DeserializationError::InvalidInput;
    incoming_.clear();
    auto error = deserializeJson(incoming_, in, DeserializationOption::NestingLimit(16));
    if (!error && (!incoming_.is<JsonObject>() || !in.blank()))
      error = DeserializationError::InvalidInput;
    return error;
  }
  bool shaped(JsonObjectConst r) const {
    if (!r["ok"].is<bool>())
      return false;
    if (!r["ok"].as<bool>()) {
      JsonObjectConst error = r["error"];
      return r.size() == 2 && error.size() == 2 && error["code"].is<const char *>() &&
             error["message"].is<const char *>();
    }
    // Each request kind has its own reply shape; a mismatch is a stray or late reply.
    size_t expected = 1;
    if (pending_ == Kind::Hello) {
      if (!r["session_id"].is<const char *>() ||
          strcmp(r["protocol_version"] | "", ProtocolVersion))
        return false;
      expected += 2;
    } else if (pending_ == Kind::Poll) {
      JsonArrayConst commands = r["commands"];
      if (commands.isNull() || commands.size() > 1 ||
          (commands.size() && !commands[0].is<JsonObjectConst>()))
        return false;
      expected += 1;
    } else if (r.containsKey("duplicate")) {
      if (!r["duplicate"].is<bool>())
        return false;
      expected += 1;
    }
    return r.size() == expected;
  }
  // Returns false when the session was reset.
  bool reply(uint32_t now) {
    auto error = parse();
    if (error) {
      if (pending_ == Kind::Poll &&
          (error == DeserializationError::NoMemory || error == DeserializationError::TooDeep) &&
          rejectUnparsable())
        return true;
      resetSession(now);
      return false;
    }
    JsonObjectConst r = incoming_.as<JsonObjectConst>();
    if (!shaped(r)) {
      resetSession(now);
      return false;
    }
    if (!r["ok"].as<bool>())
      return failed(r["error"]["code"] | "", now);
    if (pending_ != Kind::Hello)
      backoff_ = 500; // The session has carried real traffic.
    switch (pending_) {
    case Kind::Hello:
      connected_ = true;
      break;
    case Kind::Ack:
      haveAck_ = false;
      ack_.clear();
      break;
    case Kind::Event:
      queue_.pop();
      attempts_ = 0;
      break;
    case Kind::Loss:
      lost_ -= reportedLost_;
      lossSequence_ = reportedLost_ = 0;
      attempts_ = 0;
      break;
    case Kind::Poll:
      for (JsonObjectConst command : r["commands"].as<JsonArrayConst>()) {
        device_.execute(command, ack_);
        haveAck_ = true;
      }
      break;
    case Kind::State:
      break;
    }
    return true;
  }
  static bool permanent(const char *code) {
    return !strcmp(code, "duplicate_conflict") || !strcmp(code, "invalid_event") ||
           !strcmp(code, "invalid_request") || !strcmp(code, "unsupported_capability") ||
           !strcmp(code, "invalid_state");
  }
  // A rejected event never blocks polling: drop it (counted as lost) and continue.
  bool failed(const char *code, uint32_t now) {
    bool event = pending_ == Kind::Event || pending_ == Kind::Loss;
    bool drop = event && (permanent(code) || ++attempts_ >= MaxEventAttempts);
    if (drop) {
      attempts_ = 0;
      if (pending_ == Kind::Event) {
        queue_.pop();
        ++lost_;
      } else {
        lost_ -= reportedLost_;
        lossSequence_ = reportedLost_ = 0;
      }
      if (permanent(code))
        return true; // The gateway keeps the connection for these codes.
    }
    resetSession(now);
    return false;
  }
  // A legal command too large or deep for this device still gets a definite failed ACK.
  bool rejectUnparsable() {
    StaticJsonDocument<256> filter;
    filter["commands"][0]["command_id"] = true;
    Span in{lines_.value(), lines_.value() + lines_.size()};
    incoming_.clear();
    if (deserializeJson(incoming_, in, DeserializationOption::Filter(filter),
                        DeserializationOption::NestingLimit(64)) ||
        !in.blank())
      return false;
    const char *id = incoming_["commands"][0]["command_id"] | "";
    if (!validId(id))
      return false;
    if (!failedAck(id, true))
      failedAck(id, false); // Empty state means unknown state.
    haveAck_ = true;
    backoff_ = 500;
    return true;
  }
  bool failedAck(const char *id, bool withState) {
    ack_.clear();
    ack_["type"] = "ack";
    ack_["command_id"] = JsonString(id, JsonString::Copied); // `id` lives in incoming_.
    ack_["status"] = "failed";
    auto state = ack_.createNestedObject("state");
    if (withState)
      device_.state(state);
    auto failure = ack_.createNestedObject("error");
    failure["code"] = "invalid_command";
    failure["message"] = "Command exceeds device parse limits";
    return !ack_.overflowed() && measureJson(ack_) <= MaxFrame;
  }
  void resetSession(uint32_t now) {
    // A possibly executed command cannot be acknowledged into a new gateway session.
    connected_ = waiting_ = haveAck_ = false;
    ack_.clear();
    lines_.reset();
    retryAt_ = now + backoff_;
    backoff_ = backoff_ * 2 < MaxBackoffMs ? backoff_ * 2 : MaxBackoffMs;
  }

  Device device_;
  const char *model_, *firmware_, *prefix_;
  DynamicJsonDocument incoming_, outgoing_, ack_;
  Lines<> lines_;
  Queue<Pending, GROK_EVENT_QUEUE> queue_;
  const char *eventNames_[MaxCapabilities] = {};
  Schema schemas_[MaxCapabilities] = {};
  size_t eventCount_ = 0, schemaCount_ = 0;
  StateWriter writer_ = nullptr;
  void *writerContext_ = nullptr;
  Stream *port_ = nullptr;
  Button button_;
  int buttonPin_ = -1;
  bool activeLow_ = true, began_ = false, ready_ = false;
  bool connected_ = false, waiting_ = false, haveAck_ = false;
  Kind pending_ = Kind::Hello;
  uint8_t attempts_ = 0;
  uint32_t sequence_ = 0, lost_ = 0, reportedLost_ = 0, lossSequence_ = 0;
  uint32_t lastRequest_ = 0, lastState_ = 0, retryAt_ = 0, backoff_ = 500;
  char deviceId_[80] = {}, bootId_[32] = {};
};
} // namespace grok
