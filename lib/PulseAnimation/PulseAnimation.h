#pragma once

#include <cmath>
#include <cstdint>

class PulseAnimation {
 public:
  explicit PulseAnimation(uint32_t periodMs) : periodMs_(periodMs) {}

  void reset(uint32_t nowMs) {
    phaseMs_ = 0;
    lastUpdateMs_ = nowMs;
    initialized_ = true;
  }

  uint8_t levelAt(uint32_t nowMs) {
    if (!initialized_) {
      reset(nowMs);
    } else {
      // Unsigned subtraction also handles the millis() counter rolling over.
      const uint32_t elapsedMs = nowMs - lastUpdateMs_;
      phaseMs_ = (phaseMs_ + elapsedMs) % periodMs_;
      lastUpdateMs_ = nowMs;
    }

    const float phase =
        static_cast<float>(phaseMs_) / static_cast<float>(periodMs_);
    constexpr float FULL_CIRCLE_RADIANS = 6.28318530717958647692F;
    const float envelope =
        0.5F - (0.5F * std::cos(FULL_CIRCLE_RADIANS * phase));
    return static_cast<uint8_t>(std::lround(envelope * 255.0F));
  }

  uint32_t periodMs() const { return periodMs_; }
  uint32_t phaseMs() const { return phaseMs_; }

 private:
  uint32_t periodMs_;
  uint32_t phaseMs_ = 0;
  uint32_t lastUpdateMs_ = 0;
  bool initialized_ = false;
};
