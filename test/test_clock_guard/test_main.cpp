#include <ClockGuard.h>
#include <unity.h>

#include <cstdint>
#include <limits>

namespace {

using nova::ClockGuard;

constexpr uint32_t START_TIME = 1785103200;

void test_first_sample_initializes_guard() {
  ClockGuard guard(2);

  TEST_ASSERT_TRUE(guard.accept(START_TIME, 100));
  TEST_ASSERT_EQUAL_UINT32(START_TIME, guard.timestamp());
}

void test_expected_clock_progress_is_accepted() {
  ClockGuard guard(2);
  guard.reset(START_TIME, 0);

  TEST_ASSERT_TRUE(guard.accept(START_TIME + 1, 1000));
  TEST_ASSERT_TRUE(guard.accept(START_TIME + 2, 2000));
}

void test_backward_and_large_forward_jumps_are_rejected() {
  ClockGuard guard(2);
  guard.reset(START_TIME, 0);

  TEST_ASSERT_FALSE(guard.accept(START_TIME - 1, 1000));
  TEST_ASSERT_FALSE(guard.accept(START_TIME + 3600, 1000));
  TEST_ASSERT_EQUAL_UINT32(START_TIME, guard.timestamp());
}

void test_recovery_window_grows_while_samples_are_rejected() {
  ClockGuard guard(2);
  guard.reset(START_TIME, 0);

  TEST_ASSERT_FALSE(guard.accept(START_TIME + 3600, 1000));
  TEST_ASSERT_TRUE(guard.accept(START_TIME + 6, 6000));
}

void test_repeated_timestamp_does_not_shrink_recovery_window() {
  ClockGuard guard(2);
  guard.reset(START_TIME, 0);

  TEST_ASSERT_TRUE(guard.accept(START_TIME, 1000));
  TEST_ASSERT_TRUE(guard.accept(START_TIME, 2000));
  TEST_ASSERT_TRUE(guard.accept(START_TIME + 5, 5000));
}

void test_millis_rollover_preserves_elapsed_window() {
  constexpr uint32_t startMs =
      std::numeric_limits<uint32_t>::max() - 499U;
  ClockGuard guard(2);
  guard.reset(START_TIME, startMs);

  TEST_ASSERT_TRUE(guard.accept(START_TIME + 1, startMs + 1000U));
}

}  // namespace

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_first_sample_initializes_guard);
  RUN_TEST(test_expected_clock_progress_is_accepted);
  RUN_TEST(test_backward_and_large_forward_jumps_are_rejected);
  RUN_TEST(test_recovery_window_grows_while_samples_are_rejected);
  RUN_TEST(test_repeated_timestamp_does_not_shrink_recovery_window);
  RUN_TEST(test_millis_rollover_preserves_elapsed_window);
  return UNITY_END();
}
