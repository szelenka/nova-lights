#pragma once

#include <cstdint>

namespace nova {

class IntervalTimer {
 public:
  explicit IntervalTimer(uint32_t intervalMs)
      : intervalMs_(intervalMs == 0 ? 1 : intervalMs) {}

  void reset(uint32_t nowMs) {
    lastEventMs_ = nowMs;
    initialized_ = true;
  }

  bool ready(uint32_t nowMs) {
    if (!initialized_) {
      reset(nowMs);
      return true;
    }
    if (nowMs - lastEventMs_ < intervalMs_) {
      return false;
    }
    lastEventMs_ = nowMs;
    return true;
  }

  uint32_t intervalMs() const { return intervalMs_; }

 private:
  uint32_t intervalMs_;
  uint32_t lastEventMs_ = 0;
  bool initialized_ = false;
};

}  // namespace nova
