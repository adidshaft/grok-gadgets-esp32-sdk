# C124 source verification

Verified on 4 October 2026 from manufacturer sources:

| Detail | Value | Source |
| --- | --- | --- |
| Target | AtomS3 Lite, SKU C124; ESP32-S3FN8, 8MB flash, USB-C serial/download | [M5Stack product documentation](https://docs.m5stack.com/en/core/AtomS3-Lite) |
| RGB data | GPIO35, one pixel | [M5AtomS3 LedDisplay.h](https://github.com/m5stack/M5AtomS3/blob/main/src/utility/LedDisplay.h) and [M5Unified 0.2.5 pin table](https://github.com/m5stack/M5Unified/blob/0.2.5/src/M5Unified.cpp) |
| User button | GPIO41, active-low | [M5Unified 0.2.5 button sampling](https://github.com/m5stack/M5Unified/blob/0.2.5/src/M5Unified.cpp) cases board_M5AtomS3Lite in setup and update |

Firmware uses NeoPixel's supported WS2812 driver and this explicit board header rather than pulling in display/IMU libraries irrelevant to C124. Pin verification is documentation evidence, not physical verification. The product page's schematic link is shared AtomS3 family material and includes display circuitry; the explicit C124 manufacturer library is used to resolve its LED/button pins.
