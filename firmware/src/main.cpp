// Serial log lines are the evidence for build step 4 (turn-change latency):
//   RX  <rx_us> ...      when the BLE write arrived
//   DRAW <us> +<ms>      when the frame showing it finished pushing
#include <Arduino.h>

#include "ble_link.h"
#include "pod_state.h"
#include "proto.h"
#include "ui.h"

namespace {

Ui ui;
app::PodState state;
uint32_t pending_rx_us = 0;  // oldest received packet not yet on screen; 0 = none
uint32_t last_dropped = 0;

const char* resultName(proto::Result r) {
  switch (r) {
    case proto::Result::Ok: return "ok";
    case proto::Result::TooShort: return "too-short";
    case proto::Result::LengthMismatch: return "len-mismatch";
    case proto::Result::UnsupportedVersion: return "bad-version";
    case proto::Result::UnknownType: return "unknown-type";
    case proto::Result::BadPayload: return "bad-payload";
  }
  return "?";
}

void logPacket(const LinkEvent& ev, proto::Result r, const proto::Packet& p) {
  Serial.printf("RX %lu type=0x%02X len=%u %s", static_cast<unsigned long>(ev.rx_us),
                ev.len >= 2 ? ev.data[1] : 0, ev.len, resultName(r));
  if (r == proto::Result::Ok) {
    switch (p.type) {
      case proto::Type::NavText:
        Serial.printf(" seq=%u dist=%lu eta=%u rem=%lu \"%s\"", p.nav_text.icon_seq,
                      static_cast<unsigned long>(p.nav_text.dist_m), p.nav_text.eta_min,
                      static_cast<unsigned long>(p.nav_text.remaining_m), p.nav_text.instr);
        break;
      case proto::Type::NavIcon:
        Serial.printf(" seq=%u chunk=%u/%u", p.nav_icon.icon_seq, p.nav_icon.chunk_idx,
                      p.nav_icon.chunk_cnt);
        break;
      case proto::Type::Call:
        // Caller names are personal data; log only that one arrived.
        Serial.printf(" caller_len=%u", static_cast<unsigned>(strlen(p.call.caller)));
        break;
      case proto::Type::Status:
        Serial.printf(" batt=%u time=%lu off=%d", p.status.phone_batt_pct,
                      static_cast<unsigned long>(p.status.unix_time), p.status.utc_offset_min);
        break;
      case proto::Type::NavEnd:
        break;
    }
  }
  Serial.println();
}

void handle(const LinkEvent& ev) {
  const uint32_t now = millis();
  switch (ev.kind) {
    case LinkEvent::Connected:
      Serial.printf("CONN %lu\n", static_cast<unsigned long>(ev.rx_us));
      state.onConnected(true, now);
      return;
    case LinkEvent::Disconnected:
      Serial.printf("DISC %lu\n", static_cast<unsigned long>(ev.rx_us));
      state.onConnected(false, now);
      return;
    case LinkEvent::Data:
      break;
  }
  proto::Packet p;
  const proto::Result r = proto::decode(ev.data, ev.len, p);
  logPacket(ev, r, p);
  if (r != proto::Result::Ok) return;  // malformed or newer protocol: ignore, stay connected
  state.onPacket(p, now);
  if (pending_rx_us == 0) pending_rx_us = ev.rx_us | 1;  // | 1 keeps 0 meaning "none"
}

}  // namespace

void setup() {
  Serial.begin(115200);
  if (!ui.begin()) Serial.println("ERR framebuffer alloc failed; display disabled");
  ble_link::begin();
  Serial.println("READY hunter-tripper protocol v1");
}

void loop() {
  LinkEvent ev;
  while (ble_link::poll(ev)) handle(ev);

  if (ui.render(state, millis()) && pending_rx_us != 0) {
    const uint32_t now_us = micros();
    Serial.printf("DRAW %lu +%lu ms\n", static_cast<unsigned long>(now_us),
                  static_cast<unsigned long>((now_us - pending_rx_us) / 1000));
  }
  pending_rx_us = 0;

  const uint32_t d = ble_link::dropped();
  if (d != last_dropped) {
    Serial.printf("WARN dropped=%lu\n", static_cast<unsigned long>(d));
    last_dropped = d;
  }
  delay(2);
}
