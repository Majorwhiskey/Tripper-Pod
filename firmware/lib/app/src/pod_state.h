// Everything the pod knows, built from decoded packets. No hardware access, so the
// screen-selection rules are tested on the host.
#pragma once

#include <stdint.h>

#include "icon_assembler.h"
#include "proto.h"

namespace app {

enum class Screen : uint8_t {
  Clock,   // not navigating, or phone not connected
  Nav,     // navigating; draw icon only if iconReady()
  NoData,  // navigating but the phone could not parse the Maps notification
  Call,    // incoming call, shown for kCallShowMs over any other screen
};

constexpr uint32_t kCallShowMs = 15000;

class PodState {
 public:
  PodState();

  // Disconnect drops nav data: the phone resends everything on reconnect, and a
  // stale instruction is worse than none.
  void onConnected(bool connected, uint32_t now_ms);
  void onPacket(const proto::Packet& p, uint32_t now_ms);

  Screen screen(uint32_t now_ms) const;
  bool connected() const { return connected_; }
  const proto::NavText& nav() const { return nav_; }
  bool iconReady() const { return icon_.readyFor(nav_.icon_seq); }
  const uint8_t* icon() const { return icon_.bitmap(); }
  const char* caller() const { return caller_; }
  uint8_t phoneBattPct() const { return batt_pct_; }

  // Seconds since local midnight, or -1 if no STATUS with a valid time yet.
  int32_t localSecondsOfDay(uint32_t now_ms) const;

  // Incremented on every change that affects what is drawn.
  uint32_t revision() const { return revision_; }

 private:
  void clearNav();

  bool connected_ = false;
  bool navigating_ = false;
  proto::NavText nav_;
  proto::IconAssembler icon_;

  bool call_active_ = false;
  uint32_t call_rx_ms_ = 0;
  char caller_[proto::kMaxCallerLen + 1];

  uint8_t batt_pct_ = proto::kUnknownU8;
  uint32_t unix_time_ = 0;
  int16_t utc_offset_min_ = 0;
  uint32_t time_rx_ms_ = 0;

  uint32_t revision_ = 0;
};

}  // namespace app
