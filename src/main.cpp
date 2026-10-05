// SPDX-License-Identifier: Apache-2.0
// M5Stack AtomS3 Lite C124: one RGB pixel (`rgb.set`) and the front button over USB.
#include "C124.h"
#include <Adafruit_NeoPixel.h>
#include <GrokSession.h>

Adafruit_NeoPixel pixel(grok::c124::PixelCount, grok::c124::RgbPin, NEO_GRB + NEO_KHZ800);
grok::Rgb colour;
// Global or static only. Gadget is about 40 KB and overflows the 8 KB loop stack.
grok::Gadget gadget(grok::c124::Model, "0.2.0", "c124");

void reportState(JsonObject state, void *) { grok::writeRgb(state, colour); }
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
void setup() {
  pixel.begin();
  pixel.clear();
  pixel.show();
  gadget.state(reportState);
  gadget.command("rgb.set", setRgb);
  gadget.button(grok::c124::ButtonPin);
  gadget.begin();
}
void loop() { gadget.loop(); }
