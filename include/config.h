#pragma once

#include <cstdint>

#ifdef ARDUINO
#include <Arduino.h>

// External NeoPixel data pins. These defaults keep the STEMMA QT I2C pins free
// for the PCF8523. Change them to match the final QT Py wiring.
constexpr uint8_t TOP_LIGHT_PIN = A2;
constexpr uint8_t MIDDLE_LIGHT_PIN = A3;
constexpr uint8_t BOTTOM_LIGHT_PIN = A1;
#endif

constexpr uint16_t TOP_LIGHT_PIXELS = 16;
constexpr uint16_t MIDDLE_LIGHT_PIXELS = 8;
constexpr uint16_t BOTTOM_LIGHT_PIXELS = 16;

constexpr uint8_t LIGHT_BRIGHTNESS = 179;  // Approximately 70%.

// Complete dark -> bright -> dark cycle for the middle RGBW light.
constexpr uint32_t MIDDLE_PULSE_PERIOD_MS = 3000;

static_assert(TOP_LIGHT_PIXELS == 8 || TOP_LIGHT_PIXELS == 16,
              "Top light must contain 8 or 16 pixels");
static_assert(MIDDLE_LIGHT_PIXELS == 8 || MIDDLE_LIGHT_PIXELS == 16,
              "Middle light must contain 8 or 16 pixels");
static_assert(BOTTOM_LIGHT_PIXELS == 8 || BOTTOM_LIGHT_PIXELS == 16,
              "Bottom light must contain 8 or 16 pixels");
static_assert(MIDDLE_PULSE_PERIOD_MS > 0,
              "Middle pulse period must be greater than zero");
