#include <string.h>
#include <unity.h>

#include "icon_assembler.h"

using namespace proto;

void setUp() {}
void tearDown() {}

static NavIcon chunk(uint8_t seq, uint8_t idx) {
  NavIcon c = {};
  c.icon_seq = seq;
  c.chunk_idx = idx;
  c.chunk_cnt = kIconChunkCount;
  memset(c.data, seq * 16 + idx, kIconChunkLen);
  return c;
}

static void test_complete_icon_ready_for_matching_seq_only() {
  IconAssembler a;
  for (uint8_t i = 0; i < 3; ++i) TEST_ASSERT_FALSE(a.feed(chunk(5, i)));
  TEST_ASSERT_FALSE(a.readyFor(5));
  TEST_ASSERT_TRUE(a.feed(chunk(5, 3)));
  TEST_ASSERT_TRUE(a.readyFor(5));
  TEST_ASSERT_FALSE(a.readyFor(6));
  for (uint8_t i = 0; i < 4; ++i) {
    TEST_ASSERT_EACH_EQUAL_UINT8(5 * 16 + i, a.bitmap() + i * kIconChunkLen, kIconChunkLen);
  }
}

static void test_out_of_order_and_duplicate_chunks() {
  IconAssembler a;
  TEST_ASSERT_FALSE(a.feed(chunk(1, 3)));
  TEST_ASSERT_FALSE(a.feed(chunk(1, 3)));
  TEST_ASSERT_FALSE(a.feed(chunk(1, 0)));
  TEST_ASSERT_FALSE(a.feed(chunk(1, 2)));
  TEST_ASSERT_TRUE(a.feed(chunk(1, 1)));
  TEST_ASSERT_FALSE(a.feed(chunk(1, 1)));  // resend after completion is not a new icon
  TEST_ASSERT_TRUE(a.readyFor(1));
}

static void test_new_seq_discards_partial_icon() {
  IconAssembler a;
  a.feed(chunk(1, 0));
  a.feed(chunk(1, 1));
  a.feed(chunk(2, 2));
  a.feed(chunk(2, 3));
  // Chunks 0,1 of seq 1 must not combine with 2,3 of seq 2.
  TEST_ASSERT_FALSE(a.readyFor(1));
  TEST_ASSERT_FALSE(a.readyFor(2));
}

static void test_new_seq_replaces_complete_icon() {
  IconAssembler a;
  for (uint8_t i = 0; i < 4; ++i) a.feed(chunk(1, i));
  a.feed(chunk(2, 0));
  TEST_ASSERT_FALSE(a.readyFor(1));
  TEST_ASSERT_FALSE(a.readyFor(2));
}

static void test_invalid_chunk_ignored() {
  IconAssembler a;
  NavIcon bad = chunk(1, 4);
  TEST_ASSERT_FALSE(a.feed(bad));
  bad = chunk(1, 0);
  bad.chunk_cnt = 2;
  TEST_ASSERT_FALSE(a.feed(bad));
  for (uint8_t i = 0; i < 3; ++i) a.feed(chunk(1, i));
  TEST_ASSERT_FALSE(a.readyFor(1));
}

static void test_reset_clears_icon() {
  IconAssembler a;
  for (uint8_t i = 0; i < 4; ++i) a.feed(chunk(9, i));
  a.reset();
  TEST_ASSERT_FALSE(a.readyFor(9));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_complete_icon_ready_for_matching_seq_only);
  RUN_TEST(test_out_of_order_and_duplicate_chunks);
  RUN_TEST(test_new_seq_discards_partial_icon);
  RUN_TEST(test_new_seq_replaces_complete_icon);
  RUN_TEST(test_invalid_chunk_ignored);
  RUN_TEST(test_reset_clears_icon);
  return UNITY_END();
}
