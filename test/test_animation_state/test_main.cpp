#include <AnimationState.h>
#include <unity.h>

#include <array>
#include <cstdint>
#include <limits>

namespace {

using nova::AnimationState;
using nova::BLUE;
using nova::Color;
using nova::MAX_PIXELS;
using nova::OFF;
using nova::RED;

uint8_t countColor(const AnimationState& state, Color expected) {
  uint8_t count = 0;
  for (const Color color : state.colors()) {
    count += color == expected;
  }
  return count;
}

void test_timer_expires_once_without_replaying_missed_steps() {
  AnimationState state(16);
  state.reset(1000);

  TEST_ASSERT_TRUE(state.ready(1000));
  state.schedule(200);
  TEST_ASSERT_FALSE(state.ready(1100));
  TEST_ASSERT_EQUAL_UINT32(100, state.remainingMs());
  TEST_ASSERT_TRUE(state.ready(1500));
  TEST_ASSERT_EQUAL_UINT32(0, state.remainingMs());
}

void test_timer_handles_millis_rollover() {
  AnimationState state(16);
  constexpr uint32_t start = std::numeric_limits<uint32_t>::max() - 49;
  state.reset(start);
  state.schedule(100);

  TEST_ASSERT_FALSE(state.ready(start + 50));
  TEST_ASSERT_TRUE(state.ready(start + 100));
}

void test_first_cycle_step_changes_only_one_pixel_from_arbitrary_frame() {
  AnimationState state(16);
  state.reset(0);
  state.setSolid(RED);
  constexpr Color colors[] = {BLUE};

  const nova::CycleStep step =
      state.advanceCycle(colors, 1, 0, false);

  TEST_ASSERT_TRUE(step.changed);
  TEST_ASSERT_FALSE(step.completedTarget);
  TEST_ASSERT_EQUAL_UINT8(1, countColor(state, BLUE));
  TEST_ASSERT_EQUAL_UINT8(15, countColor(state, RED));
}

void test_cycle_reaches_target_one_distributed_pixel_per_expiry() {
  AnimationState state(16);
  state.reset(0);
  state.setSolid(RED);
  constexpr Color colors[] = {BLUE};

  for (uint8_t expectedBlue = 1; expectedBlue <= 16; ++expectedBlue) {
    const nova::CycleStep step =
        state.advanceCycle(colors, 1, 3, true);
    TEST_ASSERT_TRUE(step.changed);
    TEST_ASSERT_EQUAL_UINT8(expectedBlue, countColor(state, BLUE));
    TEST_ASSERT_EQUAL(expectedBlue == 16, step.completedTarget);
  }
}

void test_reset_clears_color_cycle_and_pending_timer() {
  AnimationState state(16);
  state.reset(0);
  state.setSolid(RED);
  state.schedule(2000);
  state.reset(50);

  TEST_ASSERT_TRUE(state.isOff());
  TEST_ASSERT_TRUE(state.ready(50));
}

}  // namespace

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_timer_expires_once_without_replaying_missed_steps);
  RUN_TEST(test_timer_handles_millis_rollover);
  RUN_TEST(test_first_cycle_step_changes_only_one_pixel_from_arbitrary_frame);
  RUN_TEST(test_cycle_reaches_target_one_distributed_pixel_per_expiry);
  RUN_TEST(test_reset_clears_color_cycle_and_pending_timer);
  return UNITY_END();
}
