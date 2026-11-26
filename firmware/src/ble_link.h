// BLE GATT server (decision D8). NimBLE callbacks run on the BLE host task, so they
// only copy bytes into a queue; all decoding and state changes happen in loop().
#pragma once

#include <stdint.h>

#include "proto.h"

struct LinkEvent {
  enum Kind : uint8_t { Connected, Disconnected, Data };
  Kind kind;
  uint8_t len;
  uint32_t rx_us;  // micros() when the write arrived, for latency logging
  uint8_t data[proto::kMaxPacketLen];
};

namespace ble_link {

void begin();

// Non-blocking. Returns false when nothing is pending.
bool poll(LinkEvent& out);

// Events lost because the queue was full or a write was oversized, since boot.
uint32_t dropped();

}  // namespace ble_link
