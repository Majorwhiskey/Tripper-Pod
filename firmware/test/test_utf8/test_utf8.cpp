#include <string.h>
#include <unity.h>

#include "utf8.h"

using namespace proto;

void setUp() {}
void tearDown() {}

static size_t san(const char* in, size_t in_len, char* out, size_t max) {
  return sanitizeUtf8(reinterpret_cast<const uint8_t*>(in), in_len, out, max);
}

static void test_valid_multibyte_passes_through() {
  const char* s = "25 \xC2\xB0" "C \xE0\xA4\xAE \xF0\x9F\x9A\x97";  // ° Devanagari ma, emoji
  char out[64];
  TEST_ASSERT_EQUAL_size_t(strlen(s), san(s, strlen(s), out, 63));
  TEST_ASSERT_EQUAL_STRING(s, out);
}

static void test_truncation_never_splits_code_point() {
  const char* s = "ab\xE0\xA4\xAE";  // 2 ASCII + one 3-byte char
  char out[8];
  TEST_ASSERT_EQUAL_size_t(2, san(s, 5, out, 4));
  TEST_ASSERT_EQUAL_STRING("ab", out);
  TEST_ASSERT_EQUAL_size_t(5, san(s, 5, out, 5));
}

static void test_invalid_bytes_replaced() {
  char out[16];
  // Lone continuation, overlong '/', surrogate, > U+10FFFF, truncated sequence.
  const char* cases[] = {"\x80", "\xC0\xAF", "\xED\xA0\x80", "\xF4\x90\x80\x80", "\xE0\xA4"};
  const char* expect[] = {"?", "??", "???", "????", "??"};
  for (int i = 0; i < 5; ++i) {
    san(cases[i], strlen(cases[i]), out, 15);
    TEST_ASSERT_EQUAL_STRING(expect[i], out);
  }
}

static void test_embedded_nul_replaced() {
  const char in[] = {'a', '\0', 'b'};
  char out[8];
  TEST_ASSERT_EQUAL_size_t(3, san(in, 3, out, 7));
  TEST_ASSERT_EQUAL_STRING("a?b", out);
}

static void test_prefix_len_respects_boundaries() {
  const char* s = "ab\xE0\xA4\xAE" "c";
  TEST_ASSERT_EQUAL_size_t(2, utf8PrefixLen(s, 3));
  TEST_ASSERT_EQUAL_size_t(2, utf8PrefixLen(s, 4));
  TEST_ASSERT_EQUAL_size_t(5, utf8PrefixLen(s, 5));
  TEST_ASSERT_EQUAL_size_t(6, utf8PrefixLen(s, 100));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_valid_multibyte_passes_through);
  RUN_TEST(test_truncation_never_splits_code_point);
  RUN_TEST(test_invalid_bytes_replaced);
  RUN_TEST(test_embedded_nul_replaced);
  RUN_TEST(test_prefix_len_respects_boundaries);
  return UNITY_END();
}
