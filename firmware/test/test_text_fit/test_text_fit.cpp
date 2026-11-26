#include <string.h>
#include <unity.h>

#include "text_fit.h"

using namespace app;

void setUp() {}
void tearDown() {}

// Monospace: 10 px per code point.
static int mono(const char* s, size_t len, void*) {
  int px = 0;
  for (size_t i = 0; i < len; ++i) {
    if ((static_cast<unsigned char>(s[i]) & 0xC0) != 0x80) px += 10;
  }
  return px;
}

static void test_fit_prefix() {
  const char* s = "abcdef";
  TEST_ASSERT_EQUAL_size_t(3, fitPrefix(s, 6, 35, mono, nullptr));
  TEST_ASSERT_EQUAL_size_t(6, fitPrefix(s, 6, 1000, mono, nullptr));
  TEST_ASSERT_EQUAL_size_t(0, fitPrefix(s, 6, 5, mono, nullptr));
}

static void test_fit_prefix_keeps_code_points_whole() {
  const char* s = "a\xC2\xB0" "b";  // a ° b
  TEST_ASSERT_EQUAL_size_t(1, fitPrefix(s, 4, 15, mono, nullptr));
  TEST_ASSERT_EQUAL_size_t(3, fitPrefix(s, 4, 20, mono, nullptr));
}

static void test_wrap_breaks_at_last_fitting_word() {
  const char* s = "Turn left onto MG Road";
  const size_t n = strlen(s);
  Line l = wrapLine(s, n, 100, mono, nullptr);  // 10 chars per line
  TEST_ASSERT_EQUAL_size_t(0, l.start);
  TEST_ASSERT_EQUAL_size_t(9, l.len);  // "Turn left"
  TEST_ASSERT_EQUAL_size_t(10, l.next);
  l = wrapLine(s + 10, n - 10, 100, mono, nullptr);
  TEST_ASSERT_EQUAL_size_t(7, l.len);  // "onto MG"
  TEST_ASSERT_EQUAL_size_t(8, l.next);
  l = wrapLine(s + 18, n - 18, 100, mono, nullptr);
  TEST_ASSERT_EQUAL_size_t(4, l.len);  // "Road"
  TEST_ASSERT_EQUAL_size_t(4, l.next);
}

static void test_wrap_long_word_breaks_mid_word() {
  const char* s = "Thiruvananthapuram";
  Line l = wrapLine(s, strlen(s), 50, mono, nullptr);
  TEST_ASSERT_EQUAL_size_t(5, l.len);
  TEST_ASSERT_EQUAL_size_t(5, l.next);
}

static void test_wrap_always_progresses() {
  const char* s = "W";
  Line l = wrapLine(s, 1, 0, mono, nullptr);  // nothing fits
  TEST_ASSERT_EQUAL_size_t(1, l.next);
}

static void test_wrap_skips_leading_spaces_and_handles_empty() {
  const char* s = "   Go";
  Line l = wrapLine(s, 5, 100, mono, nullptr);
  TEST_ASSERT_EQUAL_size_t(3, l.start);
  TEST_ASSERT_EQUAL_size_t(2, l.len);
  TEST_ASSERT_EQUAL_size_t(5, l.next);
  Line e = wrapLine("  ", 2, 100, mono, nullptr);
  TEST_ASSERT_EQUAL_size_t(0, e.len);
  TEST_ASSERT_EQUAL_size_t(2, e.next);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_fit_prefix);
  RUN_TEST(test_fit_prefix_keeps_code_points_whole);
  RUN_TEST(test_wrap_breaks_at_last_fitting_word);
  RUN_TEST(test_wrap_long_word_breaks_mid_word);
  RUN_TEST(test_wrap_always_progresses);
  RUN_TEST(test_wrap_skips_leading_spaces_and_handles_empty);
  return UNITY_END();
}
