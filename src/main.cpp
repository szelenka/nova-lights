#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <RTClib.h>
#include <Wire.h>

#include <algorithm>
#include <array>

#include <AnimationLogic.h>
#include <AnimationState.h>
#include <PulseAnimation.h>

#include "config.h"
#include "ValidationMode.h"

namespace {

using nova::AnimationMode;
using nova::AnimationPlan;
using nova::AnimationState;
using nova::BLUE;
using nova::Color;
using nova::CYAN;
using nova::DAY_COLORS;
using nova::GREEN;
using nova::MAX_PIXELS;
using nova::MINT;
using nova::OFF;
using nova::ORANGE;
using nova::PINK;
using nova::PURPLE;
using nova::RED;
using nova::WHITE;
using nova::WHITE_RGB;
using nova::YELLOW;
using nova::animationMode;
using nova::makeChaseFrame;
using nova::prepareHour;
using nova::rgbw;

constexpr size_t RANDOM_COLOR_COUNT = 9;

uint32_t secondsToMs(float seconds) {
  return static_cast<uint32_t>(seconds * 1000.0F);
}

float randomFloat(float lower, float upper) {
  const float unit = static_cast<float>(random(0L, 1000001L)) / 1000000.0F;
  return lower + ((upper - lower) * unit);
}

float randomDuration() {
  return randomFloat(0.0F, 1.0F) > 0.25F
             ? randomFloat(0.1F, 0.3F)
             : randomFloat(1.0F, 2.0F);
}

class NovaStar {
 public:
  NovaStar(uint16_t pixelCount, uint8_t pin, neoPixelType type,
           float fadeDuration = 0.5F)
      : pixels_(pixelCount, pin, type),
        pixelCount_(pixelCount),
        state_(pixelCount),
        pauseDurationMs_(secondsToMs(fadeDuration * 2.0F)),
        cycleDurationMs_(secondsToMs(fadeDuration / pixelCount)) {}

  void begin() {
    pixels_.begin();
    pixels_.setBrightness(LIGHT_BRIGHTNESS);
    reset();
  }

  void reset() {
    state_.reset(millis());
    pixels_.clear();
    pixels_.show();
  }

  void turnOff() {
    if (!state_.isOff()) {
      reset();
    }
  }

  void setSolid(Color color) {
    state_.makeImmediate(millis());
    state_.resetCycle();
    if (state_.setSolid(color)) {
      render();
    }
  }

  void cycleBetween(const Color* colors, size_t colorCount,
                    uint32_t cycleDurationMinMs = 0,
                    uint32_t cycleDurationMaxMs = 0,
                    uint32_t pauseDurationMinMs = 0,
                    uint32_t pauseDurationMaxMs = 0) {
    if (colorCount == 0) {
      return;
    }

    const uint32_t nowMs = millis();
    if (!state_.ready(nowMs)) {
      return;
    }

    cycleDurationMinMs =
        cycleDurationMinMs == 0 ? cycleDurationMs_ : cycleDurationMinMs;
    cycleDurationMaxMs = cycleDurationMaxMs == 0 ? cycleDurationMinMs
                                                 : cycleDurationMaxMs;
    pauseDurationMinMs =
        pauseDurationMinMs == 0 ? pauseDurationMs_ : pauseDurationMinMs;
    pauseDurationMaxMs =
        pauseDurationMaxMs == 0 ? pauseDurationMinMs : pauseDurationMaxMs;

    const nova::CycleStep step = state_.advanceCycle(
        colors, colorCount, random(0L, static_cast<long>(pixelCount_)),
        random(0L, 2L) == 1);
    state_.schedule(step.completedTarget
                        ? randomMilliseconds(pauseDurationMinMs,
                                             pauseDurationMaxMs)
                        : randomMilliseconds(cycleDurationMinMs,
                                             cycleDurationMaxMs));
    if (step.changed) {
      render();
    }
  }

  void chase(Color primary, Color secondary, uint32_t durationMs = 0,
             float percentage = 0.25F, float randomness = 0.5F,
             int movement = 1, uint8_t sections = 2) {
    const uint32_t nowMs = millis();
    if (!state_.ready(nowMs)) {
      return;
    }

    durationMs =
        durationMs == 0 ? secondsToMs(randomDuration()) : durationMs;
    const int direction =
        randomFloat(0.0F, 1.0F) <= randomness ? movement : -movement;
    const auto frame =
        makeChaseFrame(state_.colors(), pixelCount_, primary, secondary,
                       percentage, direction, movement, sections);
    state_.resetCycle();
    state_.schedule(durationMs);
    if (state_.replaceFrame(frame)) {
      render();
    }
  }

 private:
  uint32_t randomMilliseconds(uint32_t minimum, uint32_t maximum) {
    if (maximum <= minimum) {
      return minimum;
    }
    return static_cast<uint32_t>(
        random(static_cast<long>(minimum), static_cast<long>(maximum + 1)));
  }

  void render() {
    const auto& colors = state_.colors();
    for (uint16_t index = 0; index < pixelCount_; ++index) {
      pixels_.setPixelColor(index, colors[index]);
    }
    pixels_.show();
  }

  Adafruit_NeoPixel pixels_;
  uint16_t pixelCount_;
  AnimationState state_;
  uint32_t pauseDurationMs_;
  uint32_t cycleDurationMs_;
};

void shuffleColors(std::array<Color, RANDOM_COLOR_COUNT>& colors) {
  for (size_t index = colors.size() - 1; index > 0; --index) {
    const size_t swapIndex = random(0L, static_cast<long>(index + 1));
    std::swap(colors[index], colors[swapIndex]);
  }
}

std::array<Color, RANDOM_COLOR_COUNT> makeRandomColors(bool isRgbw) {
  std::array<Color, RANDOM_COLOR_COUNT> colors{
      RED, ORANGE, YELLOW, GREEN, MINT, BLUE, PURPLE, PINK,
      isRgbw ? WHITE : WHITE_RGB,
  };
  shuffleColors(colors);
  return colors;
}

RTC_PCF8523 rtc;
#ifdef NOVA_VALIDATION_MODE
ValidationMode validation;
#endif
NovaStar topLight(TOP_LIGHT_PIXELS, TOP_LIGHT_PIN, NEO_GRB + NEO_KHZ800);
NovaStar middleLight(MIDDLE_LIGHT_PIXELS, MIDDLE_LIGHT_PIN,
                     NEO_GRBW + NEO_KHZ800);
NovaStar bottomLight(BOTTOM_LIGHT_PIXELS, BOTTOM_LIGHT_PIN,
                     NEO_GRB + NEO_KHZ800);
PulseAnimation middlePulse(MIDDLE_PULSE_PERIOD_MS);

std::array<Color, RANDOM_COLOR_COUNT> topRandomColors{};
std::array<Color, RANDOM_COLOR_COUNT> middleRandomColors{};
std::array<Color, RANDOM_COLOR_COUNT> bottomRandomColors{};
bool randomCyclesReady = false;
int previousSecond = -1;

void refreshRandomColors() {
  topRandomColors = makeRandomColors(false);
  middleRandomColors = makeRandomColors(true);
  bottomRandomColors = makeRandomColors(false);
  randomCyclesReady = true;
}

void printTime(const DateTime& now) {
  char timestamp[] = "DDD MM/DD/YYYY @ hh:mm:ss";
  Serial.println(now.toString(timestamp));
}

#ifndef NOVA_VALIDATION_MODE
[[noreturn]] void haltForRtcError(const char* message) {
  topLight.turnOff();
  middleLight.turnOff();
  bottomLight.turnOff();
  while (true) {
    Serial.println(message);
    delay(5000);
  }
}
#endif

}  // namespace

void setup() {
  Serial.begin(115200);
  Wire.begin();
  randomSeed(micros());

  topLight.begin();
  middleLight.begin();
  bottomLight.begin();
  middlePulse.reset(millis());

#ifdef NOVA_VALIDATION_MODE
  validation.begin(millis());
#else
  if (!rtc.begin()) {
    haltForRtcError("PCF8523 not found on the STEMMA QT I2C bus.");
  }
#ifdef NOVA_SET_RTC_TO_BUILD_TIME
  rtc.adjust(DateTime(NOVA_RTC_YEAR, NOVA_RTC_MONTH, NOVA_RTC_DAY,
                      NOVA_RTC_HOUR, NOVA_RTC_MINUTE, NOVA_RTC_SECOND));
  Serial.println("PCF8523 set from the computer's local build time.");
#else
  if (!rtc.initialized() || rtc.lostPower()) {
    haltForRtcError(
        "PCF8523 time is invalid. Upload the set_rtc environment.");
  }
#endif
#endif
}

void loop() {
#ifdef NOVA_VALIDATION_MODE
  static size_t previousValidationScenario = validation.selected();
  validation.update(millis());
  if (validation.selected() != previousValidationScenario) {
    previousValidationScenario = validation.selected();
    randomCyclesReady = false;
    topLight.reset();
    middleLight.reset();
    bottomLight.reset();
    middlePulse.reset(millis());
  }
  const DateTime now = validation.now();
#else
  const DateTime now = rtc.now();
#endif
  if (now.second() != previousSecond) {
    previousSecond = now.second();
    printTime(now);
  }

  const AnimationPlan plan =
      prepareHour(now.hour(), now.minute(), now.second());
  const bool isFriday = now.dayOfTheWeek() == 5;
  const AnimationMode mode =
      animationMode(plan, isFriday, now.minute(), now.second());
  const bool isTransition = mode == AnimationMode::Transition;

  if ((isTransition || isFriday) && !randomCyclesReady) {
    refreshRandomColors();
  } else if (!isFriday && !isTransition) {
    randomCyclesReady = false;
  }

  if (mode == AnimationMode::Off) {
    topLight.turnOff();
    middleLight.turnOff();
    bottomLight.turnOff();
    return;
  }

  if (mode == AnimationMode::Transition) {
    topLight.cycleBetween(
        topRandomColors.data(), topRandomColors.size(), 10, 70, 100, 250);
    middleLight.cycleBetween(
        middleRandomColors.data(), middleRandomColors.size(), 10, 70, 100,
        250);
    bottomLight.cycleBetween(
        bottomRandomColors.data(), bottomRandomColors.size(), 10, 70, 100,
        250);
    return;
  }

  if (mode == AnimationMode::Friday) {
    topLight.cycleBetween(
        topRandomColors.data(), topRandomColors.size(), 10, 70, 500, 1500);
    middleLight.cycleBetween(
        middleRandomColors.data(), middleRandomColors.size(), 10, 70, 500,
        1500);
    bottomLight.cycleBetween(
        bottomRandomColors.data(), bottomRandomColors.size(), 10, 70, 500,
        1500);
    return;
  }

  topLight.chase(plan.primary, plan.secondary, plan.durationMs,
                 plan.percentage);
  middleLight.setSolid(rgbw(0, 0, 0, middlePulse.levelAt(millis())));
  bottomLight.chase(DAY_COLORS[now.dayOfTheWeek()], OFF, 0, 0.25F);
}
