#pragma once

#include <Arduino.h>

// External NeoPixel data pins. These defaults keep the STEMMA QT I2C pins free
// for the PCF8523. Change them to match the final QT Py wiring.
constexpr uint8_t TOP_LIGHT_PIN = A2;
constexpr uint8_t MIDDLE_LIGHT_PIN = A3;
constexpr uint8_t BOTTOM_LIGHT_PIN = A1;

constexpr uint16_t TOP_LIGHT_PIXELS = 16;
constexpr uint16_t MIDDLE_LIGHT_PIXELS = 8;
constexpr uint16_t BOTTOM_LIGHT_PIXELS = 16;

constexpr uint8_t LIGHT_BRIGHTNESS = 179;  // Approximately 70%.

// Complete dark -> bright -> dark cycle for the middle RGBW light.
constexpr uint32_t MIDDLE_PULSE_PERIOD_MS = 3000;

// Set true for one upload to initialize the PCF8523 from the firmware build
// timestamp, then set it back to false and upload again.
constexpr bool SET_RTC_TO_BUILD_TIME = false;
