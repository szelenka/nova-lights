#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

#include <AnimationLogic.h>

namespace nova {

constexpr size_t MAX_CONCURRENT_TWINKLES = 3;

struct Twinkle {
  bool active = false;
  uint8_t pixel = 0;
  uint32_t elapsedMs = 0;
  uint32_t durationMs = 1;
  uint8_t depth = 0;
};

class TwinkleAnimation {
 public:
  explicit TwinkleAnimation(
      uint16_t pixelCount,
      uint8_t maxActive = static_cast<uint8_t>(MAX_CONCURRENT_TWINKLES))
      : pixelCount_(pixelCount > MAX_PIXELS ? MAX_PIXELS : pixelCount),
        maxActive_(maxActive > MAX_CONCURRENT_TWINKLES
                       ? MAX_CONCURRENT_TWINKLES
                       : maxActive) {}

  void reset(uint32_t nowMs) {
    twinkles_.fill(Twinkle{});
    lastUpdateMs_ = nowMs;
    remainingUntilEventMs_ = 0;
    eventDue_ = true;
    hasLastPixel_ = false;
  }

  bool advance(uint32_t nowMs) {
    const uint32_t elapsedMs = nowMs - lastUpdateMs_;
    lastUpdateMs_ = nowMs;

    for (Twinkle& twinkle : twinkles_) {
      if (!twinkle.active) {
        continue;
      }
      if (elapsedMs >= twinkle.durationMs - twinkle.elapsedMs) {
        twinkle.active = false;
        continue;
      }
      twinkle.elapsedMs += elapsedMs;
    }

    if (!eventDue_) {
      if (elapsedMs >= remainingUntilEventMs_) {
        remainingUntilEventMs_ = 0;
        eventDue_ = true;
      } else {
        remainingUntilEventMs_ -= elapsedMs;
      }
    }
    return eventDue_;
  }

  bool startEvent(uint8_t candidatePixel, uint32_t durationMs, uint8_t depth,
                  uint32_t nextEventDelayMs) {
    if (!eventDue_) {
      return false;
    }

    eventDue_ = false;
    remainingUntilEventMs_ = nextEventDelayMs;
    if (pixelCount_ == 0 || durationMs == 0 || activeCount() >= maxActive_) {
      return false;
    }

    Twinkle* slot = firstAvailableSlot();
    if (slot == nullptr) {
      return false;
    }

    const uint8_t start = candidatePixel % pixelCount_;
    for (uint16_t offset = 0; offset < pixelCount_; ++offset) {
      const uint8_t pixel = (start + offset) % pixelCount_;
      if (!canUse(pixel)) {
        continue;
      }

      slot->active = true;
      slot->pixel = pixel;
      slot->elapsedMs = 0;
      slot->durationMs = durationMs;
      slot->depth = depth;
      lastPixel_ = pixel;
      hasLastPixel_ = true;
      return true;
    }
    return false;
  }

  std::array<Color, MAX_PIXELS> frame(Color base, Color twinkleColor) const {
    std::array<Color, MAX_PIXELS> result{};
    result.fill(base);
    for (const Twinkle& twinkle : twinkles_) {
      if (twinkle.active) {
        result[twinkle.pixel] =
            blend(base, twinkleColor, envelopeAmount(twinkle));
      }
    }
    return result;
  }

  size_t activeCount() const {
    return static_cast<size_t>(
        std::count_if(twinkles_.begin(), twinkles_.end(),
                      [](const Twinkle& twinkle) { return twinkle.active; }));
  }

  bool isActive(uint8_t pixel) const {
    return std::any_of(
        twinkles_.begin(), twinkles_.end(), [pixel](const Twinkle& twinkle) {
          return twinkle.active && twinkle.pixel == pixel;
        });
  }

  bool eventDue() const { return eventDue_; }

 private:
  static uint8_t blendChannel(uint8_t from, uint8_t to, uint8_t amount) {
    const uint16_t inverse = 255U - amount;
    return static_cast<uint8_t>(
        ((static_cast<uint32_t>(from) * inverse) +
         (static_cast<uint32_t>(to) * amount) + 127U) /
        255U);
  }

  static Color blend(Color from, Color to, uint8_t amount) {
    Color result = 0;
    for (uint8_t shift = 0; shift <= 24; shift += 8) {
      const uint8_t fromChannel = (from >> shift) & 0xFFU;
      const uint8_t toChannel = (to >> shift) & 0xFFU;
      result |= static_cast<Color>(
                    blendChannel(fromChannel, toChannel, amount))
                << shift;
    }
    return result;
  }

  static uint8_t envelopeAmount(const Twinkle& twinkle) {
    const float progress =
        static_cast<float>(twinkle.elapsedMs) / twinkle.durationMs;
    float fade = 1.0F;
    if (progress < 0.4F) {
      fade = progress / 0.4F;
    } else if (progress > 0.6F) {
      fade = (1.0F - progress) / 0.4F;
    }
    const float eased = fade * fade * (3.0F - (2.0F * fade));
    return static_cast<uint8_t>((twinkle.depth * eased) + 0.5F);
  }

  Twinkle* firstAvailableSlot() {
    const auto slot =
        std::find_if(twinkles_.begin(), twinkles_.end(),
                     [](const Twinkle& twinkle) { return !twinkle.active; });
    return slot == twinkles_.end() ? nullptr : &(*slot);
  }

  bool canUse(uint8_t pixel) const {
    if (hasLastPixel_ && pixel == lastPixel_) {
      return false;
    }
    return std::none_of(
        twinkles_.begin(), twinkles_.end(),
        [this, pixel](const Twinkle& twinkle) {
          if (!twinkle.active) {
            return false;
          }
          const uint8_t clockwise = (twinkle.pixel + 1U) % pixelCount_;
          const uint8_t counterClockwise =
              (twinkle.pixel + pixelCount_ - 1U) % pixelCount_;
          return pixel == twinkle.pixel || pixel == clockwise ||
                 pixel == counterClockwise;
        });
  }

  uint16_t pixelCount_;
  size_t maxActive_;
  std::array<Twinkle, MAX_CONCURRENT_TWINKLES> twinkles_{};
  uint32_t lastUpdateMs_ = 0;
  uint32_t remainingUntilEventMs_ = 0;
  uint8_t lastPixel_ = 0;
  bool eventDue_ = true;
  bool hasLastPixel_ = false;
};

}  // namespace nova
