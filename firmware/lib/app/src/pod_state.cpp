#include "pod_state.h"

#include <string.h>

namespace app {

PodState::PodState() {
  clearNav();
  caller_[0] = '\0';
}

void PodState::clearNav() {
  navigating_ = false;
  memset(&nav_, 0, sizeof nav_);
  nav_.dist_m = proto::kUnknownU32;
  nav_.eta_min = proto::kUnknownU16;
  nav_.remaining_m = proto::kUnknownU32;
  icon_.reset();
}

void PodState::onConnected(bool connected, uint32_t) {
  if (connected == connected_) return;
  connected_ = connected;
  if (!connected) {
    clearNav();
    call_active_ = false;
    batt_pct_ = proto::kUnknownU8;
  }
  ++revision_;
}

void PodState::onPacket(const proto::Packet& p, uint32_t now_ms) {
  switch (p.type) {
    case proto::Type::NavText:
      navigating_ = true;
      nav_ = p.nav_text;
      break;
    case proto::Type::NavIcon:
      // Only a completed icon changes what is drawn.
      if (!icon_.feed(p.nav_icon)) return;
      break;
    case proto::Type::NavEnd:
      clearNav();
      break;
    case proto::Type::Call:
      call_active_ = true;
      call_rx_ms_ = now_ms;
      memcpy(caller_, p.call.caller, sizeof caller_);
      break;
    case proto::Type::Status:
      batt_pct_ = p.status.phone_batt_pct;
      unix_time_ = p.status.unix_time;
      utc_offset_min_ = p.status.utc_offset_min;
      time_rx_ms_ = now_ms;
      break;
  }
  ++revision_;
}

Screen PodState::screen(uint32_t now_ms) const {
  // Unsigned subtraction stays correct across the 49-day millis() wrap.
  if (call_active_ && now_ms - call_rx_ms_ < kCallShowMs) return Screen::Call;
  if (!connected_ || !navigating_) return Screen::Clock;
  if (nav_.instr[0] == '\0' && nav_.dist_m == proto::kUnknownU32) return Screen::NoData;
  return Screen::Nav;
}

int32_t PodState::localSecondsOfDay(uint32_t now_ms) const {
  if (unix_time_ == 0) return -1;
  int64_t t = static_cast<int64_t>(unix_time_) + (now_ms - time_rx_ms_) / 1000 +
              static_cast<int64_t>(utc_offset_min_) * 60;
  t %= 86400;
  if (t < 0) t += 86400;
  return static_cast<int32_t>(t);
}

}  // namespace app
