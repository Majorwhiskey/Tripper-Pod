#include <unity.h>

#include "format.h"
#include "proto.h"

using namespace app;

void setUp() {}
void tearDown() {}

static char buf[32];

static const char* dist(uint32_t m) {
  formatDistance(m, buf, sizeof buf);
  return buf;
}

static const char* dur(uint16_t m) {
  formatDuration(m, buf, sizeof buf);
  return buf;
}

static const char* clk(int32_t s) {
  formatClock(s, buf, sizeof buf);
  return buf;
}

static void test_distance() {
  TEST_ASSERT_EQUAL_STRING("0 m", dist(0));
  TEST_ASSERT_EQUAL_STRING("10 m", dist(5));
  TEST_ASSERT_EQUAL_STRING("300 m", dist(300));
  TEST_ASSERT_EQUAL_STRING("350 m", dist(347));
  TEST_ASSERT_EQUAL_STRING("990 m", dist(994));
  TEST_ASSERT_EQUAL_STRING("1.0 km", dist(995));
  TEST_ASSERT_EQUAL_STRING("1.2 km", dist(1234));
  TEST_ASSERT_EQUAL_STRING("9.9 km", dist(9949));
  TEST_ASSERT_EQUAL_STRING("10 km", dist(9950));
  TEST_ASSERT_EQUAL_STRING("124 km", dist(123600));
  TEST_ASSERT_EQUAL_STRING("--", dist(proto::kUnknownU32));
  // Largest real value must not overflow the rounding.
  TEST_ASSERT_EQUAL_STRING("4294967 km", dist(proto::kUnknownU32 - 1));
}

static void test_duration() {
  TEST_ASSERT_EQUAL_STRING("0 min", dur(0));
  TEST_ASSERT_EQUAL_STRING("59 min", dur(59));
  TEST_ASSERT_EQUAL_STRING("1 h 00 min", dur(60));
  TEST_ASSERT_EQUAL_STRING("2 h 05 min", dur(125));
  TEST_ASSERT_EQUAL_STRING("--", dur(proto::kUnknownU16));
}

static void test_clock() {
  TEST_ASSERT_EQUAL_STRING("00:00", clk(0));
  TEST_ASSERT_EQUAL_STRING("09:05", clk(9 * 3600 + 5 * 60 + 59));
  TEST_ASSERT_EQUAL_STRING("23:59", clk(86399));
  TEST_ASSERT_EQUAL_STRING("--:--", clk(-1));
}

static void test_small_buffer_truncates_safely() {
  char tiny[4] = {'x', 'x', 'x', 'x'};
  formatDistance(1234, tiny, sizeof tiny);
  TEST_ASSERT_EQUAL_STRING("1.2", tiny);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_distance);
  RUN_TEST(test_duration);
  RUN_TEST(test_clock);
  RUN_TEST(test_small_buffer_truncates_safely);
  return UNITY_END();
}
