// SPDX-License-Identifier: Apache-2.0
// Climate sensor (hub ADR 013 §6): an AHT20 temperature and humidity module on I2C (SDA GPIO8,
// SCL GPIO9, 3.3 V). It reports `climate` in its state every 2 seconds and sends a
// `threshold.crossed` event when the temperature goes above or below `above_c`. Grok Bot can
// answer "how warm is it?" and "tell me when it goes above 26 degrees". A module that is
// missing at boot, or stops answering three times in a row, reports `sensor: missing`; the
// sketch looks for it again every 5 seconds, so a loose wire recovers without a reboot.
// Not tested on hardware until someone runs it with the part; host models cover the logic.
#include <GrokApp.h>
#include <Wire.h>

constexpr int SdaPin = 8, SclPin = 9;
constexpr uint8_t Aht20 = 0x38;
constexpr uint32_t SampleMs = 2000, MeasureMs = 80, RetryMs = 5000;
constexpr int MaxFailures = 3;

grok::Gadget gadget("Climate sensor", GROK_APP_VERSION, "climate");
grok::WifiApp app(gadget);
float temperature = 0, humidity = 0, aboveC = 25;
bool sensorOk = false, haveReading = false, wasAbove = false;
uint32_t sampledAt = 0, triggeredAt = 0;
bool measuring = false;
int failures = 0;

#ifdef GROK_HOST_SIMULATION
// Host model: a repeatable swing from 22 to 28 degrees and back, one degree a reading.
// Host tests unplug and replug the module with hostSensorPresent.
bool hostSensorPresent = true;
bool startMeasure() { return hostSensorPresent; }
bool readMeasure() {
  static int step = 0;
  if (!hostSensorPresent)
    return false;
  temperature = 22.0f + float(step < 6 ? step : 12 - step);
  humidity = 45.0f;
  step = (step + 1) % 12;
  return true;
}
bool beginSensor() { return hostSensorPresent; }
#else
bool beginSensor() {
  Wire.begin(SdaPin, SclPin);
  Wire.beginTransmission(Aht20);
  if (Wire.endTransmission() != 0)
    return false;
  Wire.beginTransmission(Aht20); // Calibrate (datasheet: 0xBE 0x08 0x00).
  Wire.write(0xBE);
  Wire.write(0x08);
  Wire.write(0x00);
  return Wire.endTransmission() == 0;
}
bool startMeasure() {
  Wire.beginTransmission(Aht20); // Trigger a measurement (0xAC 0x33 0x00).
  Wire.write(0xAC);
  Wire.write(0x33);
  Wire.write(0x00);
  return Wire.endTransmission() == 0;
}
bool readMeasure() {
  uint8_t raw[6];
  if (Wire.requestFrom(Aht20, uint8_t(6)) != 6)
    return false;
  for (auto &byte : raw)
    byte = Wire.read();
  if (raw[0] & 0x80)
    return false; // Still busy.
  uint32_t h = (uint32_t(raw[1]) << 12) | (uint32_t(raw[2]) << 4) | (raw[3] >> 4);
  uint32_t t = (uint32_t(raw[3] & 0x0F) << 16) | (uint32_t(raw[4]) << 8) | raw[5];
  humidity = h * 100.0f / 1048576.0f;
  temperature = t * 200.0f / 1048576.0f - 50.0f;
  return true;
}
#endif

float tenth(float value) { return float(int(value * 10 + (value < 0 ? -0.5f : 0.5f))) / 10; }

void reportState(JsonObject state, void *) {
  auto climate = state.createNestedObject("climate");
  climate["sensor"] = sensorOk ? "ok" : "missing";
  climate["above_c"] = tenth(aboveC);
  if (haveReading) {
    climate["temperature_c"] = tenth(temperature);
    climate["humidity_pct"] = tenth(humidity);
  }
}
grok::Error setThreshold(JsonObjectConst args, void *) {
  if (args.size() != 1 || !args["above_c"].is<float>())
    return {"invalid_arguments", "Expected {\"above_c\": number from -40 to 85}"};
  float value = args["above_c"];
  if (value < -40 || value > 85)
    return {"invalid_arguments", "above_c must be from -40 to 85"};
  aboveC = value;
  wasAbove = haveReading && temperature > aboveC; // No event for the change itself.
  return {};
}

void setup() {
  sensorOk = beginSensor();
  gadget.state(reportState);
  gadget.command(
      "threshold.set", setThreshold, nullptr,
      R"({"type":"object","description":"Set the temperature in degrees C above which the )"
      R"(sensor sends threshold.crossed","properties":{"above_c":{"type":"number",)"
      R"("minimum":-40,"maximum":85}},"required":["above_c"],"additionalProperties":false})");
  gadget.event("threshold.crossed",
               R"({"description":"The temperature went above or below above_c",)"
               R"("properties":{"reading":{"type":"string"},"value":{"type":"number"},)"
               R"("above":{"type":"boolean"}}})");
  gadget.button(0);
  app.begin();
}

void sensorFailed() {
  measuring = false;
  if (++failures < MaxFailures)
    return;
  sensorOk = false; // Stop reporting a stale reading; look for the module again.
  haveReading = false;
  failures = 0;
}

void loop() {
  app.loop();
  uint32_t now = millis();
  if (!sensorOk) {
    if (uint32_t(now - sampledAt) >= RetryMs) {
      sampledAt = now;
      sensorOk = beginSensor();
    }
    return;
  }
  if (!measuring && uint32_t(now - sampledAt) >= SampleMs) {
    sampledAt = now;
    triggeredAt = now;
    measuring = startMeasure();
    if (!measuring)
      sensorFailed();
  } else if (measuring && uint32_t(now - triggeredAt) >= MeasureMs) {
    measuring = false;
    if (!readMeasure()) {
      sensorFailed();
      return;
    }
    failures = 0;
    bool above = temperature > aboveC;
    if (haveReading && above != wasAbove) {
      StaticJsonDocument<128> data;
      data["reading"] = "temperature_c";
      data["value"] = tenth(temperature);
      data["above"] = above;
      gadget.emit("threshold.crossed", data.as<JsonObjectConst>()); // A full queue counts as lost.
    }
    wasAbove = above;
    haveReading = true;
  }
}
