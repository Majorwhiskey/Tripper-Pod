#include <string.h>
#include <unity.h>

#include "proto.h"
#include "vectors.h"

using namespace proto;

void setUp() {}
void tearDown() {}

static size_t encodeFields(const Vector& v, uint8_t* buf, size_t cap) {
  switch (v.type) {
    case Type::NavText: {
      NavText t = {};
      t.icon_seq = v.icon_seq;
      t.dist_m = v.dist_m;
      t.eta_min = v.eta_min;
      t.remaining_m = v.remaining_m;
      strncpy(t.instr, v.text, kMaxInstrLen);
      return encodeNavText(t, buf, cap);
    }
    case Type::NavIcon: {
      NavIcon c = {};
      c.icon_seq = v.icon_seq;
      c.chunk_idx = v.chunk_idx;
      c.chunk_cnt = v.chunk_cnt;
      memcpy(c.data, v.icon_data, kIconChunkLen);
      return encodeNavIcon(c, buf, cap);
    }
    case Type::NavEnd:
      return encodeNavEnd(buf, cap);
    case Type::Call: {
      Call c = {};
      strncpy(c.caller, v.text, kMaxCallerLen);
      return encodeCall(c, buf, cap);
    }
    case Type::Status: {
      Status s = {v.phone_batt_pct, v.unix_time, v.utc_offset_min};
      return encodeStatus(s, buf, cap);
    }
  }
  return 0;
}

static void checkDecodedFields(const Vector& v, const Packet& p) {
  TEST_ASSERT_EQUAL_MESSAGE(v.type, p.type, v.name);
  switch (v.type) {
    case Type::NavText:
      TEST_ASSERT_EQUAL_UINT8_MESSAGE(v.icon_seq, p.nav_text.icon_seq, v.name);
      TEST_ASSERT_EQUAL_UINT32_MESSAGE(v.dist_m, p.nav_text.dist_m, v.name);
      TEST_ASSERT_EQUAL_UINT16_MESSAGE(v.eta_min, p.nav_text.eta_min, v.name);
      TEST_ASSERT_EQUAL_UINT32_MESSAGE(v.remaining_m, p.nav_text.remaining_m, v.name);
      TEST_ASSERT_EQUAL_STRING_MESSAGE(v.text, p.nav_text.instr, v.name);
      break;
    case Type::NavIcon:
      TEST_ASSERT_EQUAL_UINT8_MESSAGE(v.icon_seq, p.nav_icon.icon_seq, v.name);
      TEST_ASSERT_EQUAL_UINT8_MESSAGE(v.chunk_idx, p.nav_icon.chunk_idx, v.name);
      TEST_ASSERT_EQUAL_UINT8_MESSAGE(v.chunk_cnt, p.nav_icon.chunk_cnt, v.name);
      TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(v.icon_data, p.nav_icon.data, kIconChunkLen, v.name);
      break;
    case Type::NavEnd:
      break;
    case Type::Call:
      TEST_ASSERT_EQUAL_STRING_MESSAGE(v.text, p.call.caller, v.name);
      break;
    case Type::Status:
      TEST_ASSERT_EQUAL_UINT8_MESSAGE(v.phone_batt_pct, p.status.phone_batt_pct, v.name);
      TEST_ASSERT_EQUAL_UINT32_MESSAGE(v.unix_time, p.status.unix_time, v.name);
      TEST_ASSERT_EQUAL_INT16_MESSAGE(v.utc_offset_min, p.status.utc_offset_min, v.name);
      break;
  }
}

static void test_decoder_matches_every_vector() {
  for (const Vector& v : kVectors) {
    Packet p;
    TEST_ASSERT_EQUAL_MESSAGE(v.result, decode(v.bytes, v.len, p), v.name);
    if (v.result == Result::Ok) checkDecodedFields(v, p);
  }
}

static void test_encoder_reproduces_encode_vectors() {
  size_t checked = 0;
  for (const Vector& v : kVectors) {
    if (!v.encode) continue;
    uint8_t buf[kMaxPacketLen];
    const size_t n = encodeFields(v, buf, sizeof buf);
    TEST_ASSERT_EQUAL_size_t_MESSAGE(v.len, n, v.name);
    TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(v.bytes, buf, v.len, v.name);
    ++checked;
  }
  // Guards against a generator bug that silently marks nothing as an encode vector.
  TEST_ASSERT_GREATER_OR_EQUAL_size_t(10, checked);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_decoder_matches_every_vector);
  RUN_TEST(test_encoder_reproduces_encode_vectors);
  return UNITY_END();
}
