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

constexpr uint32_t TWINKLE_DURATION_MIN_MS = 300;
constexpr uint32_t TWINKLE_DURATION_MAX_MS = 900;
constexpr uint32_t TWINKLE_GAP_MIN_MS = 180;
constexpr uint32_t TWINKLE_GAP_MAX_MS = 510;
constexpr uint8_t TWINKLE_DEPTH_MIN = 255;
constexpr uint8_t TWINKLE_DEPTH_MAX = 255;
constexpr uint8_t TWINKLE_MAX_ACTIVE = 3;

constexpr uint32_t RTC_REFRESH_PERIOD_MS = 1000;
constexpr uint32_t RTC_SAMPLE_TOLERANCE_SECONDS = 2;
constexpr uint32_t WATCHDOG_TIMEOUT_MS = 8000;

static_assert(TOP_LIGHT_PIXELS == 8 || TOP_LIGHT_PIXELS == 16,
              "Top light must contain 8 or 16 pixels");
static_assert(MIDDLE_LIGHT_PIXELS == 8 || MIDDLE_LIGHT_PIXELS == 16,
              "Middle light must contain 8 or 16 pixels");
static_assert(BOTTOM_LIGHT_PIXELS == 8 || BOTTOM_LIGHT_PIXELS == 16,
              "Bottom light must contain 8 or 16 pixels");
static_assert(MIDDLE_PULSE_PERIOD_MS > 0,
              "Middle pulse period must be greater than zero");
static_assert(TWINKLE_DURATION_MIN_MS > 0 &&
                  TWINKLE_DURATION_MIN_MS <= TWINKLE_DURATION_MAX_MS,
              "Twinkle duration range is invalid");
static_assert(TWINKLE_GAP_MIN_MS <= TWINKLE_GAP_MAX_MS,
              "Twinkle gap range is invalid");
static_assert(TWINKLE_DEPTH_MIN <= TWINKLE_DEPTH_MAX,
              "Twinkle depth range is invalid");
static_assert(TWINKLE_MAX_ACTIVE > 0 && TWINKLE_MAX_ACTIVE <= 3,
              "Twinkle active count must be between one and three");
static_assert(RTC_REFRESH_PERIOD_MS > 0,
              "RTC refresh period must be greater than zero");
static_assert(RTC_SAMPLE_TOLERANCE_SECONDS > 0,
              "RTC sample tolerance must be greater than zero");
static_assert(WATCHDOG_TIMEOUT_MS > RTC_REFRESH_PERIOD_MS,
              "Watchdog must allow at least one RTC refresh period");
