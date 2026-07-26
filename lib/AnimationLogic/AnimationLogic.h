#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace nova {

using Color = uint32_t;

constexpr Color rgb(uint8_t red, uint8_t green, uint8_t blue) {
  return (static_cast<Color>(red) << 16U) |
         (static_cast<Color>(green) << 8U) | blue;
}

constexpr Color rgbw(uint8_t red, uint8_t green, uint8_t blue,
                     uint8_t white) {
  return (static_cast<Color>(white) << 24U) | rgb(red, green, blue);
}

constexpr Color OFF = rgbw(0, 0, 0, 0);
constexpr Color WHITE = rgbw(0, 0, 0, 255);
constexpr Color YELLOW = rgb(200, 155, 0);
constexpr Color GREEN = rgb(50, 150, 50);
constexpr Color BLUE = rgb(0, 0, 100);
constexpr Color PURPLE = rgb(180, 50, 180);
constexpr Color PINK = rgb(231, 84, 128);
constexpr Color ORANGE = rgb(155, 50, 0);
constexpr Color WHITE_RGB = rgb(100, 100, 100);
constexpr Color RED = rgb(100, 5, 5);
constexpr Color CYAN = rgb(0, 100, 100);
constexpr Color MINT = rgb(62, 180, 137);

constexpr size_t MAX_PIXELS = 16;

struct HourPeriod {
  uint8_t start;
  Color color;
  Color secondary;
};

constexpr std::array<HourPeriod, 9> HOUR_PERIODS{{
    {7, BLUE, OFF},
    {9, MINT, OFF},
    {11, PINK, OFF},
    {12, GREEN, OFF},
    {13, YELLOW, OFF},
    {16, RED, OFF},
    {18, ORANGE, OFF},
    {19, PURPLE, OFF},
    {21, OFF, OFF},
}};

/*
  0 Sunday
  1 Monday
  2 Tuesday
  3 Wednesday
  4 Thursday
  5 Friday
  6 Saturday
*/
constexpr std::array<Color, 7> DAY_COLORS{
    WHITE_RGB, BLUE, CYAN, YELLOW, PINK, CYAN, RED,
};

struct AnimationPlan {
  Color primary;
  Color secondary;
  uint32_t durationMs;
  float percentage;
};

inline AnimationPlan prepareHour(uint8_t hour, uint8_t minute, uint8_t second) {
  for (size_t index = 0; index < HOUR_PERIODS.size(); ++index) {
    const HourPeriod& current = HOUR_PERIODS[index];
    const HourPeriod& next = HOUR_PERIODS[(index + 1) % HOUR_PERIODS.size()];
    const bool inPeriod =
        index == HOUR_PERIODS.size() - 1
            ? (hour >= current.start || hour < next.start)
            : (current.start <= hour && hour < next.start);
    if (!inPeriod) {
      continue;
    }

    if ((minute == 15 || minute == 30 || minute == 45) && second < 3) {
      return {current.color, current.secondary, 0, 0.75F};
    }
    if (hour == static_cast<uint8_t>((next.start + 23) % 24)) {
      if (minute == 59 && second >= 50) {
        return {next.color, current.color, 10, 0.5F};
      }
      if (minute >= 55) {
        return {current.color, next.color, 0, (minute - 55) / 5.0F};
      }
    }
    return {current.color, current.secondary, 0, 0.25F};
  }
  return {OFF, OFF, 1000, 1.0F};
}

enum class AnimationMode {
  Off,
  Transition,
  Friday,
  Normal,
};

inline AnimationMode animationMode(const AnimationPlan& plan, bool isFriday,
                                   uint8_t minute, uint8_t second) {
  if (plan.primary == OFF && plan.secondary == OFF) {
    return AnimationMode::Off;
  }
  if (minute == 59 && second >= 45) {
    return AnimationMode::Transition;
  }
  return isFriday ? AnimationMode::Friday : AnimationMode::Normal;
}

inline std::array<Color, MAX_PIXELS> makeChaseFrame(
    const std::array<Color, MAX_PIXELS>& currentColors, uint16_t pixelCount,
    Color primary, Color secondary, float percentage, int direction,
    int movement = 1, uint8_t sections = 2) {
  if (pixelCount == 0 || pixelCount > MAX_PIXELS || sections == 0) {
    return currentColors;
  }
  percentage = percentage < 0.0F ? 0.0F
                                 : (percentage > 1.0F ? 1.0F : percentage);
  sections = sections > pixelCount ? pixelCount : sections;

  std::array<bool, MAX_PIXELS> secondaryPositions{};
  uint16_t secondaryCount = 0;
  for (uint16_t index = 0; index < pixelCount; ++index) {
    if (currentColors[index] == secondary) {
      secondaryPositions[index] = true;
      ++secondaryCount;
    }
  }

  if (secondaryCount == 0 || secondaryCount == pixelCount) {
    secondaryPositions.fill(false);
    const uint16_t totalSecondary = static_cast<uint16_t>(
        (static_cast<float>(pixelCount) * percentage) + 0.5F);
    const uint16_t sectionWidth = pixelCount / sections;
    const uint16_t secondaryPerSection = totalSecondary / sections;
    const uint16_t remainder = totalSecondary % sections;
    for (uint8_t section = 0; section < sections; ++section) {
      const uint16_t sectionSize =
          secondaryPerSection + (section < remainder ? 1 : 0);
      for (uint16_t offset = 0; offset < sectionSize; ++offset) {
        secondaryPositions[(section * sectionWidth + offset) % pixelCount] =
            true;
      }
    }
  }

  std::array<Color, MAX_PIXELS> frame{};
  frame.fill(primary);
  const int signedMovement = direction >= 0 ? movement : -movement;
  for (uint16_t index = 0; index < pixelCount; ++index) {
    if (secondaryPositions[index]) {
      const int rawMoved = static_cast<int>(index) + signedMovement;
      const int moved =
          ((rawMoved % pixelCount) + pixelCount) % pixelCount;
      frame[moved] = secondary;
    }
  }
  return frame;
}

inline std::array<uint8_t, MAX_PIXELS> makeDistributedOrder(
    uint16_t pixelCount, uint8_t rotation = 0, bool reverse = false) {
  std::array<uint8_t, MAX_PIXELS> order{};
  if (pixelCount == 0 || pixelCount > MAX_PIXELS) {
    return order;
  }
  rotation %= pixelCount;
  if (pixelCount != 8 && pixelCount != 16) {
    for (uint16_t index = 0; index < pixelCount; ++index) {
      const int offset =
          reverse ? -static_cast<int>(index) : static_cast<int>(index);
      order[index] =
          (static_cast<int>(rotation) + offset + pixelCount) % pixelCount;
    }
    return order;
  }

  uint8_t bitCount = 0;
  for (uint16_t value = pixelCount; value > 1; value >>= 1) {
    ++bitCount;
  }

  for (uint16_t step = 0; step < pixelCount; ++step) {
    uint16_t source = step;
    uint16_t reversedBits = 0;
    for (uint8_t bit = 0; bit < bitCount; ++bit) {
      reversedBits = (reversedBits << 1U) | (source & 1U);
      source >>= 1U;
    }
    const int offset = reverse ? -static_cast<int>(reversedBits)
                               : static_cast<int>(reversedBits);
    order[step] =
        (static_cast<int>(rotation) + offset + pixelCount) % pixelCount;
  }
  return order;
}

}  // namespace nova
