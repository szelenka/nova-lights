#include <AnimationLogic.h>
#include <unity.h>

#include <array>

namespace {

using namespace nova;

template <size_t N>
void assertSecondaryAt(const std::array<Color, MAX_PIXELS>& frame,
                       const std::array<uint8_t, N>& secondaryIndexes,
                       Color primary, Color secondary) {
  for (uint8_t index = 0; index < MAX_PIXELS; ++index) {
    bool expectedSecondary = false;
    for (const uint8_t secondaryIndex : secondaryIndexes) {
      expectedSecondary |= index == secondaryIndex;
    }
    TEST_ASSERT_EQUAL_UINT32(expectedSecondary ? secondary : primary,
                             frame[index]);
  }
}

void test_top_chase_initializes_two_evenly_spaced_sections() {
  std::array<Color, MAX_PIXELS> current{};
  current.fill(OFF);

  const auto frame =
      makeChaseFrame(current, 16, RED, OFF, 0.25F, 1);
  assertSecondaryAt(frame, std::array<uint8_t, 4>{1, 2, 9, 10}, RED, OFF);
}

void test_top_chase_moves_both_directions_and_wraps() {
  std::array<Color, MAX_PIXELS> current{};
  current.fill(RED);
  for (const uint8_t index : {0, 1, 8, 9}) {
    current[index] = OFF;
  }

  const auto clockwise =
      makeChaseFrame(current, 16, RED, OFF, 0.25F, -1);
  assertSecondaryAt(clockwise, std::array<uint8_t, 4>{15, 0, 7, 8}, RED, OFF);

  const auto counterClockwise =
      makeChaseFrame(current, 16, RED, OFF, 0.25F, 1);
  assertSecondaryAt(counterClockwise, std::array<uint8_t, 4>{1, 2, 9, 10},
                    RED, OFF);
}

void test_bottom_chase_uses_the_requested_weekday_color() {
  std::array<Color, MAX_PIXELS> current{};
  current.fill(OFF);

  for (uint8_t weekday = 0; weekday < DAY_COLORS.size(); ++weekday) {
    const auto frame =
        makeChaseFrame(current, 16, DAY_COLORS[weekday], OFF, 0.25F, 1);
    TEST_ASSERT_EQUAL_UINT32(DAY_COLORS[weekday], frame[0]);
    TEST_ASSERT_EQUAL_UINT32(OFF, frame[1]);
  }
}

void test_weekday_color_mapping_is_sunday_first() {
  constexpr std::array<Color, 7> expected{
      WHITE_RGB, BLUE, CYAN, YELLOW, PINK, CYAN, RED,
  };
  for (size_t weekday = 0; weekday < expected.size(); ++weekday) {
    TEST_ASSERT_EQUAL_UINT32(expected[weekday], DAY_COLORS[weekday]);
  }
}

void test_hour_schedule_selects_normal_quarter_and_blended_periods() {
  const AnimationPlan normal = prepareHour(10, 10, 0);
  TEST_ASSERT_EQUAL_UINT32(MINT, normal.primary);
  TEST_ASSERT_EQUAL_UINT32(OFF, normal.secondary);
  TEST_ASSERT_FLOAT_WITHIN(0.001F, 0.25F, normal.percentage);

  const AnimationPlan quarter = prepareHour(10, 15, 1);
  TEST_ASSERT_FLOAT_WITHIN(0.001F, 0.75F, quarter.percentage);

  const AnimationPlan blend = prepareHour(10, 57, 0);
  TEST_ASSERT_EQUAL_UINT32(MINT, blend.primary);
  TEST_ASSERT_EQUAL_UINT32(PINK, blend.secondary);
  TEST_ASSERT_FLOAT_WITHIN(0.001F, 0.4F, blend.percentage);

  const AnimationPlan handoff = prepareHour(10, 59, 55);
  TEST_ASSERT_EQUAL_UINT32(PINK, handoff.primary);
  TEST_ASSERT_EQUAL_UINT32(MINT, handoff.secondary);
  TEST_ASSERT_EQUAL_UINT32(10, handoff.durationMs);
}

void test_quarter_hour_uses_75_percent_total_ring_coverage() {
  std::array<Color, MAX_PIXELS> current{};
  current.fill(OFF);

  const auto frame =
      makeChaseFrame(current, 16, MINT, OFF, 0.75F, 1);
  uint8_t primaryCount = 0;
  uint8_t secondaryCount = 0;
  for (const Color color : frame) {
    primaryCount += color == MINT;
    secondaryCount += color == OFF;
  }

  TEST_ASSERT_EQUAL_UINT8(4, primaryCount);
  TEST_ASSERT_EQUAL_UINT8(12, secondaryCount);
}

void test_animation_mode_prioritizes_off_transition_and_friday() {
  const AnimationPlan normal{RED, OFF, 200, 0.25F};
  const AnimationPlan off{OFF, OFF, 200, 0.25F};

  TEST_ASSERT_EQUAL_INT(static_cast<int>(AnimationMode::Off),
                        static_cast<int>(animationMode(off, true, 59, 50)));
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(AnimationMode::Transition),
      static_cast<int>(animationMode(normal, true, 59, 50)));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(AnimationMode::Friday),
                        static_cast<int>(animationMode(normal, true, 30, 0)));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(AnimationMode::Normal),
                        static_cast<int>(animationMode(normal, false, 30, 0)));

  const AnimationPlan handoff = prepareHour(10, 59, 55);
  TEST_ASSERT_EQUAL_INT(
      static_cast<int>(AnimationMode::Transition),
      static_cast<int>(animationMode(handoff, false, 59, 55)));
}

void test_top_twinkles_except_during_accents_and_color_blends() {
  const AnimationPlan normal = prepareHour(10, 10, 0);
  const AnimationPlan quarter = prepareHour(10, 15, 1);
  const AnimationPlan blend = prepareHour(10, 57, 0);

  TEST_ASSERT_FALSE(usesTopChase(normal, 10, 0));
  TEST_ASSERT_TRUE(usesTopChase(quarter, 15, 1));
  TEST_ASSERT_TRUE(usesTopChase(blend, 57, 0));
}

void test_invalid_chase_configuration_preserves_current_frame() {
  std::array<Color, MAX_PIXELS> current{};
  current.fill(RED);

  TEST_ASSERT_EQUAL_MEMORY(
      current.data(),
      makeChaseFrame(current, MAX_PIXELS + 1, BLUE, OFF, 0.25F, 1).data(),
      sizeof(current));
  TEST_ASSERT_EQUAL_MEMORY(
      current.data(),
      makeChaseFrame(current, 16, BLUE, OFF, 0.25F, 1, 1, 0).data(),
      sizeof(current));
}

void assertDistributedOrder(uint16_t pixelCount, uint8_t rotation,
                            bool reverse) {
  const auto order = makeDistributedOrder(pixelCount, rotation, reverse);
  std::array<bool, MAX_PIXELS> seen{};

  for (uint16_t step = 0; step < pixelCount; ++step) {
    TEST_ASSERT_LESS_THAN_UINT16(pixelCount, order[step]);
    TEST_ASSERT_FALSE(seen[order[step]]);
    seen[order[step]] = true;

    if (step > 0) {
      const uint8_t previous = order[step - 1];
      const uint8_t current = order[step];
      const uint8_t clockwise =
          (current + pixelCount - previous) % pixelCount;
      const uint8_t circularDistance =
          clockwise < pixelCount - clockwise ? clockwise
                                             : pixelCount - clockwise;
      TEST_ASSERT_GREATER_THAN_UINT8(1, circularDistance);
    }
  }
}

void test_ring_color_transitions_are_distributed_not_adjacent() {
  assertDistributedOrder(16, 0, false);
  assertDistributedOrder(16, 7, true);
  assertDistributedOrder(8, 0, false);
  assertDistributedOrder(8, 3, true);
}

}  // namespace

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_top_chase_initializes_two_evenly_spaced_sections);
  RUN_TEST(test_top_chase_moves_both_directions_and_wraps);
  RUN_TEST(test_bottom_chase_uses_the_requested_weekday_color);
  RUN_TEST(test_weekday_color_mapping_is_sunday_first);
  RUN_TEST(test_hour_schedule_selects_normal_quarter_and_blended_periods);
  RUN_TEST(test_quarter_hour_uses_75_percent_total_ring_coverage);
  RUN_TEST(test_animation_mode_prioritizes_off_transition_and_friday);
  RUN_TEST(test_top_twinkles_except_during_accents_and_color_blends);
  RUN_TEST(test_invalid_chase_configuration_preserves_current_frame);
  RUN_TEST(test_ring_color_transitions_are_distributed_not_adjacent);
  return UNITY_END();
}
