#include <string.h>
#include <unity.h>

#include "proto.h"

using namespace proto;

void setUp() {}
void tearDown() {}

static void test_nav_text_known_bytes() {
  // Hand-built per docs/protocol.md, independent of the encoder.
  const uint8_t pkt[] = {0x01, 0x01, 0x0F, 0x00,              // ver, type, len=15
                         0x07,                                // icon_seq
                         0x2C, 0x01, 0x00, 0x00,              // dist_m = 300
                         0x0C, 0x00,                          // eta_min = 12
                         0x10, 0x27, 0x00, 0x00,              // remaining_m = 10000
                         'L', 'e', 'f', 't'};
  Packet p;
  TEST_ASSERT_EQUAL(Result::Ok, decode(pkt, sizeof pkt, p));
  TEST_ASSERT_EQUAL(Type::NavText, p.type);
  TEST_ASSERT_EQUAL_UINT8(7, p.nav_text.icon_seq);
  TEST_ASSERT_EQUAL_UINT32(300, p.nav_text.dist_m);
  TEST_ASSERT_EQUAL_UINT16(12, p.nav_text.eta_min);
  TEST_ASSERT_EQUAL_UINT32(10000, p.nav_text.remaining_m);
  TEST_ASSERT_EQUAL_STRING("Left", p.nav_text.instr);
}

static void test_nav_text_round_trip_with_unknown_sentinels() {
  NavText in = {};
  in.icon_seq = 255;
  in.dist_m = kUnknownU32;
  in.eta_min = kUnknownU16;
  in.remaining_m = kUnknownU32;
  strcpy(in.instr, "Turn right onto MG Road");
  uint8_t buf[kMaxPacketLen];
  size_t n = encodeNavText(in, buf, sizeof buf);
  TEST_ASSERT_EQUAL_size_t(kHeaderLen + 11 + strlen(in.instr), n);
  Packet p;
  TEST_ASSERT_EQUAL(Result::Ok, decode(buf, n, p));
  TEST_ASSERT_EQUAL_UINT32(kUnknownU32, p.nav_text.dist_m);
  TEST_ASSERT_EQUAL_UINT16(kUnknownU16, p.nav_text.eta_min);
  TEST_ASSERT_EQUAL_STRING(in.instr, p.nav_text.instr);
}

static void test_nav_text_long_instruction_truncated_on_decode() {
  uint8_t buf[kMaxPacketLen] = {0x01, 0x01};
  const size_t payload = 11 + 150;
  buf[2] = payload & 0xFF;
  buf[3] = payload >> 8;
  memset(buf + kHeaderLen + 11, 'a', 150);
  Packet p;
  TEST_ASSERT_EQUAL(Result::Ok, decode(buf, kHeaderLen + payload, p));
  TEST_ASSERT_EQUAL_size_t(kMaxInstrLen, strlen(p.nav_text.instr));
}

static void test_nav_icon_round_trip() {
  NavIcon in = {};
  in.icon_seq = 3;
  in.chunk_idx = 2;
  in.chunk_cnt = kIconChunkCount;
  for (size_t i = 0; i < kIconChunkLen; ++i) in.data[i] = static_cast<uint8_t>(i * 7);
  uint8_t buf[kMaxPacketLen];
  size_t n = encodeNavIcon(in, buf, sizeof buf);
  TEST_ASSERT_EQUAL_size_t(135, n);
  Packet p;
  TEST_ASSERT_EQUAL(Result::Ok, decode(buf, n, p));
  TEST_ASSERT_EQUAL(Type::NavIcon, p.type);
  TEST_ASSERT_EQUAL_UINT8(3, p.nav_icon.icon_seq);
  TEST_ASSERT_EQUAL_UINT8(2, p.nav_icon.chunk_idx);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(in.data, p.nav_icon.data, kIconChunkLen);
}

static void test_nav_icon_bad_chunk_fields_rejected() {
  NavIcon in = {};
  in.chunk_cnt = kIconChunkCount;
  uint8_t buf[kMaxPacketLen];
  size_t n = encodeNavIcon(in, buf, sizeof buf);
  Packet p;

  buf[kHeaderLen + 1] = 4;  // chunk_idx out of range
  TEST_ASSERT_EQUAL(Result::BadPayload, decode(buf, n, p));
  buf[kHeaderLen + 1] = 0;
  buf[kHeaderLen + 2] = 3;  // chunk_cnt != 4
  TEST_ASSERT_EQUAL(Result::BadPayload, decode(buf, n, p));

  // Data shorter than 128 B, with len consistent with the bytes sent.
  buf[kHeaderLen + 2] = kIconChunkCount;
  buf[2] = 3 + 100;
  TEST_ASSERT_EQUAL(Result::BadPayload, decode(buf, kHeaderLen + 3 + 100, p));
}

static void test_nav_end_call_status_round_trip() {
  uint8_t buf[kMaxPacketLen];
  Packet p;

  size_t n = encodeNavEnd(buf, sizeof buf);
  TEST_ASSERT_EQUAL_size_t(kHeaderLen, n);
  TEST_ASSERT_EQUAL(Result::Ok, decode(buf, n, p));
  TEST_ASSERT_EQUAL(Type::NavEnd, p.type);

  Call c = {};
  strcpy(c.caller, "Amma");
  n = encodeCall(c, buf, sizeof buf);
  TEST_ASSERT_EQUAL(Result::Ok, decode(buf, n, p));
  TEST_ASSERT_EQUAL(Type::Call, p.type);
  TEST_ASSERT_EQUAL_STRING("Amma", p.call.caller);

  Status s = {kUnknownU8, 1795000000u, -210};  // negative offset exercises sign
  n = encodeStatus(s, buf, sizeof buf);
  TEST_ASSERT_EQUAL(Result::Ok, decode(buf, n, p));
  TEST_ASSERT_EQUAL(Type::Status, p.type);
  TEST_ASSERT_EQUAL_UINT8(kUnknownU8, p.status.phone_batt_pct);
  TEST_ASSERT_EQUAL_UINT32(1795000000u, p.status.unix_time);
  TEST_ASSERT_EQUAL_INT16(-210, p.status.utc_offset_min);
}

static void test_short_and_mismatched_lengths() {
  Packet p;
  const uint8_t hdr_only[] = {0x01, 0x01, 0x00};
  TEST_ASSERT_EQUAL(Result::TooShort, decode(hdr_only, sizeof hdr_only, p));
  TEST_ASSERT_EQUAL(Result::TooShort, decode(hdr_only, 0, p));
  TEST_ASSERT_EQUAL(Result::TooShort, decode(nullptr, 10, p));

  // Header claims 15 payload bytes, only 4 arrived.
  const uint8_t truncated[] = {0x01, 0x01, 0x0F, 0x00, 1, 2, 3, 4};
  TEST_ASSERT_EQUAL(Result::LengthMismatch, decode(truncated, sizeof truncated, p));

  // Header claims 0 bytes but more arrived.
  const uint8_t extra[] = {0x01, 0x03, 0x00, 0x00, 0xAA};
  TEST_ASSERT_EQUAL(Result::LengthMismatch, decode(extra, sizeof extra, p));
}

static void test_payload_shorter_than_fixed_fields() {
  Packet p;
  const uint8_t nav[] = {0x01, 0x01, 0x05, 0x00, 1, 2, 3, 4, 5};
  TEST_ASSERT_EQUAL(Result::BadPayload, decode(nav, sizeof nav, p));
  const uint8_t status[] = {0x01, 0x20, 0x06, 0x00, 50, 0, 0, 0, 0, 0};
  TEST_ASSERT_EQUAL(Result::BadPayload, decode(status, sizeof status, p));
}

static void test_unknown_version_and_type_reported_not_fatal() {
  Packet p;
  const uint8_t v2[] = {0x02, 0x03, 0x00, 0x00};
  TEST_ASSERT_EQUAL(Result::UnsupportedVersion, decode(v2, sizeof v2, p));
  const uint8_t t99[] = {0x01, 0x99, 0x01, 0x00, 0x00};
  TEST_ASSERT_EQUAL(Result::UnknownType, decode(t99, sizeof t99, p));
  // A valid packet still decodes afterwards: the decoder holds no state.
  const uint8_t end[] = {0x01, 0x03, 0x00, 0x00};
  TEST_ASSERT_EQUAL(Result::Ok, decode(end, sizeof end, p));
}

static void test_trailing_bytes_on_fixed_types_ignored() {
  Packet p;
  const uint8_t status[] = {0x01, 0x20, 0x08, 0x00, 80, 1, 0, 0, 0, 0x4A, 0x01, 0xEE};
  TEST_ASSERT_EQUAL(Result::Ok, decode(status, sizeof status, p));
  TEST_ASSERT_EQUAL_UINT8(80, p.status.phone_batt_pct);
  TEST_ASSERT_EQUAL_UINT32(1, p.status.unix_time);
  TEST_ASSERT_EQUAL_INT16(330, p.status.utc_offset_min);
}

static void test_encoder_rejects_small_buffer() {
  uint8_t buf[kHeaderLen + 6];
  Status s = {50, 0, 0};
  TEST_ASSERT_EQUAL_size_t(0, encodeStatus(s, buf, sizeof buf));
  TEST_ASSERT_EQUAL_size_t(0, encodeNavEnd(buf, kHeaderLen - 1));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_nav_text_known_bytes);
  RUN_TEST(test_nav_text_round_trip_with_unknown_sentinels);
  RUN_TEST(test_nav_text_long_instruction_truncated_on_decode);
  RUN_TEST(test_nav_icon_round_trip);
  RUN_TEST(test_nav_icon_bad_chunk_fields_rejected);
  RUN_TEST(test_nav_end_call_status_round_trip);
  RUN_TEST(test_short_and_mismatched_lengths);
  RUN_TEST(test_payload_shorter_than_fixed_fields);
  RUN_TEST(test_unknown_version_and_type_reported_not_fatal);
  RUN_TEST(test_trailing_bytes_on_fixed_types_ignored);
  RUN_TEST(test_encoder_rejects_small_buffer);
  return UNITY_END();
}
