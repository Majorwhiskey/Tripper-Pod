#include <string.h>
#include <unity.h>

#include "pod_state.h"

using namespace app;
using proto::Packet;
using proto::Type;

void setUp() {}
void tearDown() {}

static Packet navText(uint8_t seq, uint32_t dist, const char* instr) {
  Packet p = {};
  p.type = Type::NavText;
  p.nav_text.icon_seq = seq;
  p.nav_text.dist_m = dist;
  p.nav_text.eta_min = 10;
  p.nav_text.remaining_m = 5000;
  strcpy(p.nav_text.instr, instr);
  return p;
}

static Packet iconChunk(uint8_t seq, uint8_t idx) {
  Packet p = {};
  p.type = Type::NavIcon;
  p.nav_icon.icon_seq = seq;
  p.nav_icon.chunk_idx = idx;
  p.nav_icon.chunk_cnt = proto::kIconChunkCount;
  return p;
}

static Packet simple(Type t) {
  Packet p = {};
  p.type = t;
  return p;
}

static Packet status(uint32_t unix_time, int16_t offset_min) {
  Packet p = simple(Type::Status);
  p.status.phone_batt_pct = 80;
  p.status.unix_time = unix_time;
  p.status.utc_offset_min = offset_min;
  return p;
}

static void test_starts_on_clock() {
  PodState s;
  TEST_ASSERT_EQUAL(Screen::Clock, s.screen(0));
  TEST_ASSERT_EQUAL_INT32(-1, s.localSecondsOfDay(0));
}

static void test_nav_text_shows_nav_without_icon_until_complete() {
  PodState s;
  s.onConnected(true, 0);
  s.onPacket(navText(1, 300, "Left"), 0);
  TEST_ASSERT_EQUAL(Screen::Nav, s.screen(0));
  TEST_ASSERT_FALSE(s.iconReady());
  for (uint8_t i = 0; i < 4; ++i) s.onPacket(iconChunk(1, i), 0);
  TEST_ASSERT_TRUE(s.iconReady());
}

static void test_icon_for_previous_text_not_shown() {
  PodState s;
  s.onConnected(true, 0);
  for (uint8_t i = 0; i < 4; ++i) s.onPacket(iconChunk(1, i), 0);
  s.onPacket(navText(2, 300, "Right"), 0);
  TEST_ASSERT_FALSE(s.iconReady());
}

static void test_unparsed_notification_shows_no_data() {
  PodState s;
  s.onConnected(true, 0);
  s.onPacket(navText(0, proto::kUnknownU32, ""), 0);
  TEST_ASSERT_EQUAL(Screen::NoData, s.screen(0));
  // Instruction alone, distance unknown: still useful, show it.
  s.onPacket(navText(0, proto::kUnknownU32, "Head north"), 0);
  TEST_ASSERT_EQUAL(Screen::Nav, s.screen(0));
}

static void test_nav_end_returns_to_clock() {
  PodState s;
  s.onConnected(true, 0);
  s.onPacket(navText(1, 300, "Left"), 0);
  s.onPacket(simple(Type::NavEnd), 0);
  TEST_ASSERT_EQUAL(Screen::Clock, s.screen(0));
}

static void test_disconnect_drops_nav_keeps_time() {
  PodState s;
  s.onConnected(true, 0);
  s.onPacket(status(1000, 0), 0);
  s.onPacket(navText(1, 300, "Left"), 0);
  for (uint8_t i = 0; i < 4; ++i) s.onPacket(iconChunk(1, i), 0);
  s.onConnected(false, 0);
  TEST_ASSERT_EQUAL(Screen::Clock, s.screen(0));
  TEST_ASSERT_FALSE(s.iconReady());
  TEST_ASSERT_EQUAL_INT32(1000, s.localSecondsOfDay(0));
  // Reconnect without resent data must not resurrect the old instruction.
  s.onConnected(true, 0);
  TEST_ASSERT_EQUAL(Screen::Clock, s.screen(0));
}

static void test_call_overlay_expires() {
  PodState s;
  s.onConnected(true, 0);
  s.onPacket(navText(1, 300, "Left"), 0);
  Packet c = simple(Type::Call);
  strcpy(c.call.caller, "Amma");
  s.onPacket(c, 1000);
  TEST_ASSERT_EQUAL(Screen::Call, s.screen(1000));
  TEST_ASSERT_EQUAL_STRING("Amma", s.caller());
  TEST_ASSERT_EQUAL(Screen::Call, s.screen(1000 + kCallShowMs - 1));
  TEST_ASSERT_EQUAL(Screen::Nav, s.screen(1000 + kCallShowMs));
}

static void test_call_expiry_across_millis_wrap() {
  PodState s;
  s.onConnected(true, 0);
  s.onPacket(simple(Type::Call), 0xFFFFFF00u);
  TEST_ASSERT_EQUAL(Screen::Call, s.screen(0x100));
  TEST_ASSERT_EQUAL(Screen::Clock, s.screen(kCallShowMs));
}

static void test_local_time_with_offset_and_elapsed() {
  PodState s;
  s.onConnected(true, 0);
  // 2026-11-27 06:30:00 UTC = 12:00 IST.
  s.onPacket(status(1795761000u, 330), 5000);
  TEST_ASSERT_EQUAL_INT32(12 * 3600, s.localSecondsOfDay(5000));
  TEST_ASSERT_EQUAL_INT32(12 * 3600 + 90, s.localSecondsOfDay(5000 + 90500));
  // Negative offset crossing midnight backwards: 00:30 UTC at -60 min = 23:30.
  s.onPacket(status(1795739400u, -60), 0);
  TEST_ASSERT_EQUAL_INT32(23 * 3600 + 30 * 60, s.localSecondsOfDay(0));
}

static void test_revision_tracks_visible_changes_only() {
  PodState s;
  uint32_t r = s.revision();
  s.onConnected(false, 0);
  TEST_ASSERT_EQUAL_UINT32(r, s.revision());
  s.onConnected(true, 0);
  TEST_ASSERT_NOT_EQUAL(r, s.revision());
  r = s.revision();
  s.onPacket(iconChunk(1, 0), 0);
  TEST_ASSERT_EQUAL_UINT32(r, s.revision());
  s.onPacket(navText(1, 300, "Left"), 0);
  TEST_ASSERT_NOT_EQUAL(r, s.revision());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_starts_on_clock);
  RUN_TEST(test_nav_text_shows_nav_without_icon_until_complete);
  RUN_TEST(test_icon_for_previous_text_not_shown);
  RUN_TEST(test_unparsed_notification_shows_no_data);
  RUN_TEST(test_nav_end_returns_to_clock);
  RUN_TEST(test_disconnect_drops_nav_keeps_time);
  RUN_TEST(test_call_overlay_expires);
  RUN_TEST(test_call_expiry_across_millis_wrap);
  RUN_TEST(test_local_time_with_offset_and_elapsed);
  RUN_TEST(test_revision_tracks_visible_changes_only);
  return UNITY_END();
}
