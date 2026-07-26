#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <RTClib.h>
#include <Wire.h>

#include <algorithm>
#include <array>

#include <AnimationLogic.h>
#include <PulseAnimation.h>

#include "config.h"
#include "ValidationMode.h"

namespace {

using namespace nova;

struct ColorDuration {
  Color color;
  uint32_t durationMs;
};

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
        pauseDurationMs_(secondsToMs(fadeDuration * 2.0F)),
        cycleDurationMs_(secondsToMs(fadeDuration / pixelCount)) {
    durationsMs_.fill(1);
  }

  void begin() {
    pixels_.begin();
    pixels_.setBrightness(LIGHT_BRIGHTNESS);
    turnOff();
    lastTickMs_ = millis();
  }

  void turnOff() {
    currentColors_.fill(OFF);
    durationsMs_.fill(0);
    lastTickMs_ = millis();
    cycleIndex_ = 0;
    cycleColorCount_ = 0;
    transitionOrderReady_ = false;
    pixels_.clear();
    pixels_.show();
  }

  void setSolid(Color color) {
    bool refresh = false;
    for (uint16_t index = 0; index < pixelCount_; ++index) {
      if (currentColors_[index] != color) {
        currentColors_[index] = color;
        pixels_.setPixelColor(index, color);
        refresh = true;
      }
    }
    if (refresh) {
      pixels_.show();
    }
  }

  void cycleBetween(const Color* colors, size_t colorCount,
                    uint32_t cycleDurationMs = 0,
                    uint32_t pauseDurationMs = 0) {
    if (colorCount == 0) {
      return;
    }

    cycleColorCount_ = colorCount;
    cycleDurationMs =
        cycleDurationMs == 0 ? cycleDurationMs_ : cycleDurationMs;
    pauseDurationMs =
        pauseDurationMs == 0 ? pauseDurationMs_ : pauseDurationMs;

    if (cycleIndex_ >= colorCount) {
      cycleIndex_ = 0;
    }
    const Color decrementColor = colors[cycleIndex_];
    const Color incrementColor = colors[(cycleIndex_ + 1) % colorCount];
    prepareTransitionOrder(decrementColor, incrementColor);
    const uint16_t colored = countPixelsColored(incrementColor);

    std::array<ColorDuration, MAX_PIXELS> next{};
    if (colored < pixelCount_ - 1) {
      for (uint16_t index = 0; index < pixelCount_; ++index) {
        next[index] = {decrementColor, cycleDurationMs};
      }
      for (uint16_t step = 0; step <= colored; ++step) {
        next[transitionOrder_[step]] = {incrementColor, cycleDurationMs};
      }
    } else {
      for (uint16_t index = 0; index < pixelCount_; ++index) {
        next[index] = {incrementColor, pauseDurationMs};
      }
    }
    tick(next);
  }

  void chase(Color primary, Color secondary, uint32_t durationMs = 0,
             float percentage = 0.25F, float randomness = 0.5F,
             int movement = 1, uint8_t sections = 2) {
    durationMs = durationMs == 0 ? cycleDurationMs_ : durationMs;
    const int direction =
        randomFloat(0.0F, 1.0F) <= randomness ? movement : -movement;
    const auto frame =
        makeChaseFrame(currentColors_, pixelCount_, primary, secondary,
                       percentage, direction, movement, sections);

    std::array<ColorDuration, MAX_PIXELS> next{};
    for (uint16_t index = 0; index < pixelCount_; ++index) {
      next[index] = {frame[index], durationMs};
    }
    tick(next);
  }

  uint16_t pixelCount() const { return pixelCount_; }

 private:
  void prepareTransitionOrder(Color decrementColor, Color incrementColor) {
    if (transitionOrderReady_ && decrementColor == transitionFrom_ &&
        incrementColor == transitionTo_) {
      return;
    }

    transitionFrom_ = decrementColor;
    transitionTo_ = incrementColor;
    transitionOrder_ = makeDistributedOrder(
        pixelCount_, random(0L, static_cast<long>(pixelCount_)),
        random(0L, 2L) == 1);
    transitionOrderReady_ = true;
  }

  uint16_t countPixelsColored(Color color) const {
    uint16_t count = 0;
    for (uint16_t index = 0; index < pixelCount_; ++index) {
      if (currentColors_[index] == color) {
        ++count;
      }
    }
    return count;
  }

  void tick(const std::array<ColorDuration, MAX_PIXELS>& next) {
    const uint32_t now = millis();
    const uint32_t elapsed = now - lastTickMs_;
    bool refresh = false;
    bool multipleActiveColors = false;
    Color activeColor = OFF;
    bool hasActiveColor = false;

    for (uint16_t index = 0; index < pixelCount_; ++index) {
      if (elapsed >= durationsMs_[index]) {
        durationsMs_[index] = next[index].durationMs;
        if (!hasActiveColor) {
          activeColor = next[index].color;
          hasActiveColor = true;
        } else if (activeColor != next[index].color) {
          multipleActiveColors = true;
        }

        if (currentColors_[index] != next[index].color) {
          currentColors_[index] = next[index].color;
          pixels_.setPixelColor(index, next[index].color);
          refresh = true;
        }
      } else {
        durationsMs_[index] -= elapsed;
      }
    }

    if (refresh && !multipleActiveColors && cycleColorCount_ > 0) {
      cycleIndex_ = (cycleIndex_ + 1) % cycleColorCount_;
    }
    if (refresh) {
      pixels_.show();
    }
    lastTickMs_ = now;
  }

  Adafruit_NeoPixel pixels_;
  uint16_t pixelCount_;
  std::array<Color, MAX_PIXELS> currentColors_{};
  std::array<uint32_t, MAX_PIXELS> durationsMs_{};
  uint32_t lastTickMs_ = 0;
  size_t cycleIndex_ = 0;
  size_t cycleColorCount_ = 0;
  std::array<uint8_t, MAX_PIXELS> transitionOrder_{};
  Color transitionFrom_ = OFF;
  Color transitionTo_ = OFF;
  bool transitionOrderReady_ = false;
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
    Serial.println("PCF8523 not found on the STEMMA QT I2C bus.");
    while (true) {
      delay(1000);
    }
  }
  if (SET_RTC_TO_BUILD_TIME) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    Serial.println("PCF8523 set from the firmware build timestamp.");
  } else if (!rtc.initialized() || rtc.lostPower()) {
    Serial.println(
        "PCF8523 time is not set. Enable SET_RTC_TO_BUILD_TIME for one upload.");
  }
#endif
}

void loop() {
#ifdef NOVA_VALIDATION_MODE
  static size_t previousValidationScenario = validation.selected();
  validation.update(millis());
  if (validation.selected() != previousValidationScenario) {
    previousValidationScenario = validation.selected();
    randomCyclesReady = false;
    topLight.turnOff();
    middleLight.turnOff();
    bottomLight.turnOff();
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
      prepareHour(now.hour(), now.minute(), now.second(),
                  secondsToMs(randomDuration()));
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
        topRandomColors.data(), topRandomColors.size(),
        secondsToMs(randomFloat(0.01F, 0.07F)),
        secondsToMs(randomFloat(0.1F, 0.25F)));
    middleLight.cycleBetween(
        middleRandomColors.data(), middleRandomColors.size(),
        secondsToMs(randomFloat(0.01F, 0.07F)),
        secondsToMs(randomFloat(0.1F, 0.25F)));
    bottomLight.cycleBetween(
        bottomRandomColors.data(), bottomRandomColors.size(),
        secondsToMs(randomFloat(0.01F, 0.07F)),
        secondsToMs(randomFloat(0.1F, 0.25F)));
    return;
  }

  if (mode == AnimationMode::Friday) {
    topLight.cycleBetween(
        topRandomColors.data(), topRandomColors.size(),
        secondsToMs(randomFloat(0.01F, 0.07F)),
        secondsToMs(randomFloat(0.5F, 1.5F)));
    middleLight.cycleBetween(
        middleRandomColors.data(), middleRandomColors.size(),
        secondsToMs(randomFloat(0.01F, 0.07F)),
        secondsToMs(randomFloat(0.5F, 1.5F)));
    bottomLight.cycleBetween(
        bottomRandomColors.data(), bottomRandomColors.size(),
        secondsToMs(randomFloat(0.01F, 0.07F)),
        secondsToMs(randomFloat(0.5F, 1.5F)));
    return;
  }

  topLight.chase(plan.primary, plan.secondary, plan.durationMs,
                 plan.percentage);
  middleLight.setSolid(rgbw(0, 0, 0, middlePulse.levelAt(millis())));
  bottomLight.chase(DAY_COLORS[now.dayOfTheWeek()], OFF,
                    secondsToMs(randomDuration()), 0.25F);
}
