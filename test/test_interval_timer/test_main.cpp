#include <IntervalTimer.h>
#include <unity.h>

#include <cstdint>
#include <limits>

namespace {

using nova::IntervalTimer;

void test_uninitialized_timer_is_immediately_ready() {
  IntervalTimer timer(1000);

  TEST_ASSERT_TRUE(timer.ready(500));
  TEST_ASSERT_FALSE(timer.ready(1499));
  TEST_ASSERT_TRUE(timer.ready(1500));
}

void test_reset_waits_for_complete_interval() {
  IntervalTimer timer(1000);
  timer.reset(200);

  TEST_ASSERT_FALSE(timer.ready(1199));
  TEST_ASSERT_TRUE(timer.ready(1200));
}

void test_delayed_update_fires_once_without_catch_up() {
  IntervalTimer timer(1000);
  timer.reset(0);

  TEST_ASSERT_TRUE(timer.ready(5000));
  TEST_ASSERT_FALSE(timer.ready(5000));
  TEST_ASSERT_FALSE(timer.ready(5999));
  TEST_ASSERT_TRUE(timer.ready(6000));
}

void test_millis_rollover_preserves_interval() {
  constexpr uint32_t start = std::numeric_limits<uint32_t>::max() - 499U;
  IntervalTimer timer(1000);
  timer.reset(start);

  TEST_ASSERT_FALSE(timer.ready(start + 999U));
  TEST_ASSERT_TRUE(timer.ready(start + 1000U));
}

void test_zero_interval_is_clamped() {
  IntervalTimer timer(0);

  TEST_ASSERT_EQUAL_UINT32(1, timer.intervalMs());
}

}  // namespace

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_uninitialized_timer_is_immediately_ready);
  RUN_TEST(test_reset_waits_for_complete_interval);
  RUN_TEST(test_delayed_update_fires_once_without_catch_up);
  RUN_TEST(test_millis_rollover_preserves_interval);
  RUN_TEST(test_zero_interval_is_clamped);
  return UNITY_END();
}
