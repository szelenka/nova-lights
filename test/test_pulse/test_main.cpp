#include <PulseAnimation.h>
#include <unity.h>

#include "config.h"

#include <array>
#include <cstdint>
#include <limits>

namespace {

constexpr uint32_t PERIOD_MS = MIDDLE_PULSE_PERIOD_MS;

void test_key_frames_are_dark_bright_dark() {
  PulseAnimation pulse(PERIOD_MS);
  pulse.reset(0);

  TEST_ASSERT_EQUAL_UINT32(MIDDLE_PULSE_PERIOD_MS, pulse.periodMs());
  TEST_ASSERT_EQUAL_UINT8(0, pulse.levelAt(0));
  TEST_ASSERT_EQUAL_UINT8(128, pulse.levelAt(PERIOD_MS / 4));
  TEST_ASSERT_EQUAL_UINT8(255, pulse.levelAt(PERIOD_MS / 2));
  TEST_ASSERT_EQUAL_UINT8(128, pulse.levelAt((PERIOD_MS * 3) / 4));
  TEST_ASSERT_EQUAL_UINT8(0, pulse.levelAt(PERIOD_MS));
}

void test_brightness_rises_then_falls_monotonically() {
  PulseAnimation pulse(PERIOD_MS);
  pulse.reset(0);

  uint8_t previous = pulse.levelAt(0);
  for (uint32_t now = 20; now <= PERIOD_MS / 2; now += 20) {
    const uint8_t current = pulse.levelAt(now);
    TEST_ASSERT_GREATER_OR_EQUAL_UINT8(previous, current);
    previous = current;
  }

  for (uint32_t now = (PERIOD_MS / 2) + 20; now <= PERIOD_MS; now += 20) {
    const uint8_t current = pulse.levelAt(now);
    TEST_ASSERT_LESS_OR_EQUAL_UINT8(previous, current);
    previous = current;
  }
}

void test_irregular_updates_do_not_create_extra_direction_changes() {
  PulseAnimation pulse(PERIOD_MS);
  pulse.reset(0);

  const std::array<uint32_t, 15> updateTimes{
      0,
      PERIOD_MS / 100,
      PERIOD_MS / 20,
      PERIOD_MS / 6,
      PERIOD_MS / 3,
      (PERIOD_MS / 2) - 3,
      (PERIOD_MS / 2) + 4,
      (PERIOD_MS * 3) / 5,
      (PERIOD_MS * 3) / 4,
      (PERIOD_MS * 9) / 10,
      PERIOD_MS - 2,
      PERIOD_MS + (PERIOD_MS / 100),
      PERIOD_MS + (PERIOD_MS / 5),
      PERIOD_MS + (PERIOD_MS * 2) / 5,
      PERIOD_MS + (PERIOD_MS / 2) - 1,
  };

  int previousDirection = 0;
  uint8_t previousLevel = pulse.levelAt(updateTimes.front());
  unsigned directionChanges = 0;

  for (size_t index = 1; index < updateTimes.size(); ++index) {
    const uint8_t level = pulse.levelAt(updateTimes[index]);
    const int direction =
        level > previousLevel ? 1 : (level < previousLevel ? -1 : 0);
    if (direction != 0 && previousDirection != 0 &&
        direction != previousDirection) {
      ++directionChanges;
    }
    if (direction != 0) {
      previousDirection = direction;
    }
    previousLevel = level;
  }

  // One reversal at the bright peak and one at the dark cycle boundary.
  TEST_ASSERT_EQUAL_UINT(2, directionChanges);
}

void test_long_delay_jumps_to_current_phase_without_catch_up_steps() {
  PulseAnimation pulse(PERIOD_MS);
  pulse.reset(0);

  TEST_ASSERT_EQUAL_UINT8(0, pulse.levelAt(0));
  TEST_ASSERT_UINT8_WITHIN(1, 37, pulse.levelAt((PERIOD_MS * 7) / 8));
  TEST_ASSERT_UINT8_WITHIN(1, 24, pulse.levelAt((PERIOD_MS * 9) / 10));
  TEST_ASSERT_UINT8_WITHIN(1, 6, pulse.levelAt((PERIOD_MS * 19) / 20));
}

void test_millis_rollover_preserves_the_pulse() {
  PulseAnimation pulse(PERIOD_MS);
  constexpr uint32_t start = std::numeric_limits<uint32_t>::max() - 249;
  pulse.reset(start);

  TEST_ASSERT_EQUAL_UINT8(0, pulse.levelAt(start));
  TEST_ASSERT_EQUAL_UINT8(128, pulse.levelAt(start + (PERIOD_MS / 4)));
  TEST_ASSERT_EQUAL_UINT8(255, pulse.levelAt(start + (PERIOD_MS / 2)));
  TEST_ASSERT_EQUAL_UINT8(0, pulse.levelAt(start + PERIOD_MS));
}

void test_zero_period_is_safely_clamped() {
  PulseAnimation pulse(0);
  pulse.reset(0);

  TEST_ASSERT_EQUAL_UINT32(1, pulse.periodMs());
  TEST_ASSERT_EQUAL_UINT8(0, pulse.levelAt(0));
  TEST_ASSERT_EQUAL_UINT8(0, pulse.levelAt(1));
}

}  // namespace

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_key_frames_are_dark_bright_dark);
  RUN_TEST(test_brightness_rises_then_falls_monotonically);
  RUN_TEST(test_irregular_updates_do_not_create_extra_direction_changes);
  RUN_TEST(test_long_delay_jumps_to_current_phase_without_catch_up_steps);
  RUN_TEST(test_millis_rollover_preserves_the_pulse);
  RUN_TEST(test_zero_period_is_safely_clamped);
  return UNITY_END();
}
