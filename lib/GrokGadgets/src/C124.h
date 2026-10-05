// SPDX-License-Identifier: Apache-2.0
#pragma once
namespace grok {
namespace c124 {
// GPIO35. Generic ESP32-S3 and m5stack_atoms3 variants set RGB_BUILTIN and
// LED_BUILTIN to GPIO48. Use RgbPin; those macros drive the wrong pin on C124.
constexpr int RgbPin = 35;
constexpr int ButtonPin = 41; // Active-low, board pull-up.
constexpr int PixelCount = 1;
constexpr const char *Model = "M5Stack AtomS3 Lite C124";
} // namespace c124
} // namespace grok
