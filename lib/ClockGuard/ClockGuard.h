#pragma once

#include <cstdint>

namespace nova {

class ClockGuard {
 public:
  explicit ClockGuard(uint32_t toleranceSeconds)
      : toleranceSeconds_(toleranceSeconds) {}

  void reset(uint32_t timestamp, uint32_t nowMs) {
    timestamp_ = timestamp;
    acceptedAtMs_ = nowMs;
    initialized_ = true;
  }

  bool accept(uint32_t candidateTimestamp, uint32_t nowMs) {
    if (!initialized_) {
      reset(candidateTimestamp, nowMs);
      return true;
    }
    if (candidateTimestamp < timestamp_) {
      return false;
    }
    if (candidateTimestamp == timestamp_) {
      return true;
    }

    const uint32_t elapsedSeconds = (nowMs - acceptedAtMs_) / 1000U;
    const uint32_t maximumAdvance = elapsedSeconds + toleranceSeconds_;
    if (candidateTimestamp - timestamp_ > maximumAdvance) {
      return false;
    }

    reset(candidateTimestamp, nowMs);
    return true;
  }

  uint32_t timestamp() const { return timestamp_; }

 private:
  uint32_t toleranceSeconds_;
  uint32_t timestamp_ = 0;
  uint32_t acceptedAtMs_ = 0;
  bool initialized_ = false;
};

}  // namespace nova
