#include <TwinkleAnimation.h>
#include <unity.h>

#include "config.h"

#include <cstdint>
#include <limits>

namespace {

using nova::Color;
using nova::OFF;
using nova::TwinkleAnimation;
using nova::rgb;

constexpr Color BASE = rgb(100, 80, 60);

uint8_t red(Color color) {
  return static_cast<uint8_t>((color >> 16U) & 0xFFU);
}

void test_reset_requests_exactly_one_event() {
  TwinkleAnimation twinkle(16);
  twinkle.reset(100);

  TEST_ASSERT_TRUE(twinkle.eventDue());
  TEST_ASSERT_TRUE(twinkle.advance(100));
  TEST_ASSERT_TRUE(twinkle.startEvent(3, 600, 180, 200));
  TEST_ASSERT_FALSE(twinkle.eventDue());
  TEST_ASSERT_FALSE(twinkle.advance(299));
  TEST_ASSERT_TRUE(twinkle.advance(300));
}

void test_twinkle_fades_smoothly_out_and_back() {
  TwinkleAnimation twinkle(16);
  twinkle.reset(0);
  TEST_ASSERT_TRUE(
      twinkle.startEvent(3, 800, TWINKLE_DEPTH_MAX, 1000));

  const uint8_t start = red(twinkle.frame(BASE, OFF)[3]);
  twinkle.advance(200);
  const uint8_t attack = red(twinkle.frame(BASE, OFF)[3]);
  twinkle.advance(350);
  const uint8_t darkPlateauStart = red(twinkle.frame(BASE, OFF)[3]);
  twinkle.advance(450);
  const uint8_t darkPlateauEnd = red(twinkle.frame(BASE, OFF)[3]);
  twinkle.advance(600);
  const uint8_t release = red(twinkle.frame(BASE, OFF)[3]);
  twinkle.advance(800);
  const uint8_t finished = red(twinkle.frame(BASE, OFF)[3]);

  TEST_ASSERT_EQUAL_UINT8(100, start);
  TEST_ASSERT_TRUE(attack < start);
  TEST_ASSERT_EQUAL_UINT8(0, darkPlateauStart);
  TEST_ASSERT_EQUAL_UINT8(0, darkPlateauEnd);
  TEST_ASSERT_UINT8_WITHIN(1, attack, release);
  TEST_ASSERT_EQUAL_UINT8(100, finished);
}

void test_concurrent_twinkles_are_not_adjacent() {
  TwinkleAnimation twinkle(16, TWINKLE_MAX_ACTIVE);
  twinkle.reset(0);

  TEST_ASSERT_TRUE(twinkle.startEvent(0, 900, 180, 0));
  twinkle.advance(1);
  TEST_ASSERT_TRUE(twinkle.startEvent(1, 900, 180, 0));
  twinkle.advance(2);
  TEST_ASSERT_TRUE(twinkle.startEvent(15, 900, 180, 0));

  TEST_ASSERT_EQUAL_UINT8(3, twinkle.activeCount());
  TEST_ASSERT_TRUE(twinkle.isActive(0));
  TEST_ASSERT_TRUE(twinkle.isActive(2));
  TEST_ASSERT_TRUE(twinkle.isActive(4));
}

void test_recent_pixel_is_not_immediately_reused() {
  TwinkleAnimation twinkle(16);
  twinkle.reset(0);
  TEST_ASSERT_TRUE(twinkle.startEvent(5, 100, 180, 100));

  twinkle.advance(100);
  TEST_ASSERT_TRUE(twinkle.startEvent(5, 100, 180, 100));
  TEST_ASSERT_FALSE(twinkle.isActive(5));
  TEST_ASSERT_TRUE(twinkle.isActive(6));
}

void test_delayed_update_does_not_replay_missed_events() {
  TwinkleAnimation twinkle(16);
  twinkle.reset(0);
  TEST_ASSERT_TRUE(twinkle.startEvent(0, 2000, 180, 100));

  TEST_ASSERT_TRUE(twinkle.advance(1000));
  TEST_ASSERT_TRUE(twinkle.startEvent(4, 2000, 180, 100));
  TEST_ASSERT_FALSE(twinkle.advance(1000));
  TEST_ASSERT_EQUAL_UINT8(2, twinkle.activeCount());
  TEST_ASSERT_TRUE(twinkle.advance(1100));
}

void test_millis_rollover_preserves_timing() {
  constexpr uint32_t start = std::numeric_limits<uint32_t>::max() - 49U;
  TwinkleAnimation twinkle(16);
  twinkle.reset(start);
  TEST_ASSERT_TRUE(twinkle.startEvent(7, 100, 180, 100));

  TEST_ASSERT_FALSE(twinkle.advance(start + 50U));
  TEST_ASSERT_TRUE(twinkle.isActive(7));
  TEST_ASSERT_TRUE(twinkle.advance(start + 100U));
  TEST_ASSERT_FALSE(twinkle.isActive(7));
}

}  // namespace

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_reset_requests_exactly_one_event);
  RUN_TEST(test_twinkle_fades_smoothly_out_and_back);
  RUN_TEST(test_concurrent_twinkles_are_not_adjacent);
  RUN_TEST(test_recent_pixel_is_not_immediately_reused);
  RUN_TEST(test_delayed_update_does_not_replay_missed_events);
  RUN_TEST(test_millis_rollover_preserves_timing);
  return UNITY_END();
}
