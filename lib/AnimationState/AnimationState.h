#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <AnimationLogic.h>

namespace nova {

struct CycleStep {
  bool changed;
  bool completedTarget;
};

class AnimationState {
 public:
  explicit AnimationState(uint16_t pixelCount) : pixelCount_(pixelCount) {}

  void reset(uint32_t nowMs) {
    currentColors_.fill(OFF);
    remainingMs_ = 0;
    lastUpdateMs_ = nowMs;
    resetCycle();
  }

  bool ready(uint32_t nowMs) {
    const uint32_t elapsedMs = nowMs - lastUpdateMs_;
    lastUpdateMs_ = nowMs;
    if (elapsedMs >= remainingMs_) {
      remainingMs_ = 0;
      return true;
    }
    remainingMs_ -= elapsedMs;
    return false;
  }

  void schedule(uint32_t durationMs) { remainingMs_ = durationMs; }

  void makeImmediate(uint32_t nowMs) {
    remainingMs_ = 0;
    lastUpdateMs_ = nowMs;
  }

  bool setSolid(Color color) {
    bool changed = false;
    for (uint16_t index = 0; index < pixelCount_; ++index) {
      if (currentColors_[index] != color) {
        currentColors_[index] = color;
        changed = true;
      }
    }
    return changed;
  }

  bool replaceFrame(const std::array<Color, MAX_PIXELS>& frame) {
    bool changed = false;
    for (uint16_t index = 0; index < pixelCount_; ++index) {
      if (currentColors_[index] != frame[index]) {
        currentColors_[index] = frame[index];
        changed = true;
      }
    }
    return changed;
  }

  CycleStep advanceCycle(const Color* colors, size_t colorCount,
                         uint8_t rotation, bool reverse) {
    if (colorCount == 0) {
      return {false, false};
    }
    if (cycleColorCount_ != colorCount) {
      resetCycle();
      cycleColorCount_ = colorCount;
    }

    const Color target = colors[cycleIndex_];
    if (!transitionOrderReady_) {
      transitionOrder_ =
          makeDistributedOrder(pixelCount_, rotation, reverse);
      transitionStep_ = 0;
      transitionOrderReady_ = true;
    }

    while (transitionStep_ < pixelCount_ &&
           currentColors_[transitionOrder_[transitionStep_]] == target) {
      ++transitionStep_;
    }

    bool changed = false;
    if (transitionStep_ < pixelCount_) {
      currentColors_[transitionOrder_[transitionStep_]] = target;
      ++transitionStep_;
      changed = true;
    }

    while (transitionStep_ < pixelCount_ &&
           currentColors_[transitionOrder_[transitionStep_]] == target) {
      ++transitionStep_;
    }

    const bool completedTarget = transitionStep_ >= pixelCount_;
    if (completedTarget) {
      cycleIndex_ = (cycleIndex_ + 1) % colorCount;
      transitionOrderReady_ = false;
    }
    return {changed, completedTarget};
  }

  void resetCycle() {
    cycleIndex_ = 0;
    cycleColorCount_ = 0;
    transitionStep_ = 0;
    transitionOrderReady_ = false;
  }

  bool isOff() const {
    for (uint16_t index = 0; index < pixelCount_; ++index) {
      if (currentColors_[index] != OFF) {
        return false;
      }
    }
    return true;
  }

  const std::array<Color, MAX_PIXELS>& colors() const {
    return currentColors_;
  }

  uint32_t remainingMs() const { return remainingMs_; }

 private:
  uint16_t pixelCount_;
  std::array<Color, MAX_PIXELS> currentColors_{};
  uint32_t remainingMs_ = 0;
  uint32_t lastUpdateMs_ = 0;
  size_t cycleIndex_ = 0;
  size_t cycleColorCount_ = 0;
  std::array<uint8_t, MAX_PIXELS> transitionOrder_{};
  uint16_t transitionStep_ = 0;
  bool transitionOrderReady_ = false;
};

}  // namespace nova
