// Packet codec for BLE protocol v1. Spec: docs/protocol.md
#pragma once

#include <stddef.h>
#include <stdint.h>

namespace proto {

constexpr uint8_t kVersion = 1;

constexpr size_t kHeaderLen = 4;
constexpr size_t kMaxPacketLen = 244;  // ATT payload at MTU 247
constexpr size_t kMaxInstrLen = 96;
constexpr size_t kMaxCallerLen = 48;
constexpr size_t kIconChunkLen = 128;
constexpr uint8_t kIconChunkCount = 4;
constexpr size_t kIconBytes = kIconChunkLen * kIconChunkCount;

constexpr uint32_t kUnknownU32 = 0xFFFFFFFF;
constexpr uint16_t kUnknownU16 = 0xFFFF;
constexpr uint8_t kUnknownU8 = 0xFF;

enum class Type : uint8_t {
  NavText = 0x01,
  NavIcon = 0x02,
  NavEnd = 0x03,
  Call = 0x10,
  Status = 0x20,
};

struct NavText {
  uint8_t icon_seq;
  uint32_t dist_m;
  uint16_t eta_min;
  uint32_t remaining_m;
  char instr[kMaxInstrLen + 1];  // NUL-terminated, sanitised UTF-8
};

struct NavIcon {
  uint8_t icon_seq;
  uint8_t chunk_idx;
  uint8_t chunk_cnt;
  uint8_t data[kIconChunkLen];
};

struct Call {
  char caller[kMaxCallerLen + 1];  // NUL-terminated, sanitised UTF-8
};

struct Status {
  uint8_t phone_batt_pct;
  uint32_t unix_time;  // UTC seconds; 0 = unknown
  int16_t utc_offset_min;
};

struct Packet {
  Type type;
  union {
    NavText nav_text;
    NavIcon nav_icon;
    Call call;
    Status status;
  };
};

enum class Result {
  Ok,
  TooShort,            // fewer than kHeaderLen bytes
  LengthMismatch,      // header len disagrees with bytes received
  UnsupportedVersion,  // ignore, keep connection
  UnknownType,         // ignore, keep connection
  BadPayload,          // payload malformed for its type; drop
};

// Never reads past buf[len). `out` is only meaningful when Ok is returned.
Result decode(const uint8_t* buf, size_t len, Packet& out);

// Encoders return bytes written, or 0 if `cap` is too small. Strings longer than
// their limit are truncated at a code-point boundary.
size_t encodeNavText(const NavText& p, uint8_t* buf, size_t cap);
size_t encodeNavIcon(const NavIcon& p, uint8_t* buf, size_t cap);
size_t encodeNavEnd(uint8_t* buf, size_t cap);
size_t encodeCall(const Call& p, uint8_t* buf, size_t cap);
size_t encodeStatus(const Status& p, uint8_t* buf, size_t cap);

}  // namespace proto
