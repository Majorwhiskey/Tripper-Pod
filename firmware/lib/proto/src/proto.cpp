#include "proto.h"

#include <string.h>

#include "utf8.h"

namespace proto {
namespace {

constexpr size_t kNavTextFixed = 1 + 4 + 2 + 4;
constexpr size_t kNavIconLen = 3 + kIconChunkLen;
constexpr size_t kStatusFixed = 1 + 4 + 2;

uint16_t rdU16(const uint8_t* p) { return static_cast<uint16_t>(p[0] | (p[1] << 8)); }

uint32_t rdU32(const uint8_t* p) {
  return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
         (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
}

void wrU16(uint8_t* p, uint16_t v) {
  p[0] = static_cast<uint8_t>(v);
  p[1] = static_cast<uint8_t>(v >> 8);
}

void wrU32(uint8_t* p, uint32_t v) {
  for (int i = 0; i < 4; ++i) p[i] = static_cast<uint8_t>(v >> (8 * i));
}

// Writes the header and returns the payload pointer, or nullptr if it won't fit.
uint8_t* begin(Type t, size_t payload_len, uint8_t* buf, size_t cap) {
  if (payload_len > kMaxPacketLen - kHeaderLen || cap < kHeaderLen + payload_len) return nullptr;
  buf[0] = kVersion;
  buf[1] = static_cast<uint8_t>(t);
  wrU16(buf + 2, static_cast<uint16_t>(payload_len));
  return buf + kHeaderLen;
}

Result decodeNavText(const uint8_t* p, size_t n, NavText& out) {
  if (n < kNavTextFixed) return Result::BadPayload;
  out.icon_seq = p[0];
  out.dist_m = rdU32(p + 1);
  out.eta_min = rdU16(p + 5);
  out.remaining_m = rdU32(p + 7);
  sanitizeUtf8(p + kNavTextFixed, n - kNavTextFixed, out.instr, kMaxInstrLen);
  return Result::Ok;
}

Result decodeNavIcon(const uint8_t* p, size_t n, NavIcon& out) {
  if (n != kNavIconLen) return Result::BadPayload;
  if (p[2] != kIconChunkCount || p[1] >= kIconChunkCount) return Result::BadPayload;
  out.icon_seq = p[0];
  out.chunk_idx = p[1];
  out.chunk_cnt = p[2];
  memcpy(out.data, p + 3, kIconChunkLen);
  return Result::Ok;
}

}  // namespace

Result decode(const uint8_t* buf, size_t len, Packet& out) {
  if (buf == nullptr || len < kHeaderLen) return Result::TooShort;
  const size_t payload_len = rdU16(buf + 2);
  if (len != kHeaderLen + payload_len) return Result::LengthMismatch;
  if (buf[0] != kVersion) return Result::UnsupportedVersion;

  const uint8_t* p = buf + kHeaderLen;
  // Trailing bytes beyond the known fields are ignored so v1 can grow compatibly.
  switch (static_cast<Type>(buf[1])) {
    case Type::NavText:
      out.type = Type::NavText;
      return decodeNavText(p, payload_len, out.nav_text);
    case Type::NavIcon:
      out.type = Type::NavIcon;
      return decodeNavIcon(p, payload_len, out.nav_icon);
    case Type::NavEnd:
      out.type = Type::NavEnd;
      return Result::Ok;
    case Type::Call:
      out.type = Type::Call;
      sanitizeUtf8(p, payload_len, out.call.caller, kMaxCallerLen);
      return Result::Ok;
    case Type::Status:
      if (payload_len < kStatusFixed) return Result::BadPayload;
      out.type = Type::Status;
      out.status.phone_batt_pct = p[0];
      out.status.unix_time = rdU32(p + 1);
      out.status.utc_offset_min = static_cast<int16_t>(rdU16(p + 5));
      return Result::Ok;
  }
  return Result::UnknownType;
}

size_t encodeNavText(const NavText& t, uint8_t* buf, size_t cap) {
  const size_t slen = utf8PrefixLen(t.instr, kMaxInstrLen);
  uint8_t* p = begin(Type::NavText, kNavTextFixed + slen, buf, cap);
  if (p == nullptr) return 0;
  p[0] = t.icon_seq;
  wrU32(p + 1, t.dist_m);
  wrU16(p + 5, t.eta_min);
  wrU32(p + 7, t.remaining_m);
  memcpy(p + kNavTextFixed, t.instr, slen);
  return kHeaderLen + kNavTextFixed + slen;
}

size_t encodeNavIcon(const NavIcon& c, uint8_t* buf, size_t cap) {
  uint8_t* p = begin(Type::NavIcon, kNavIconLen, buf, cap);
  if (p == nullptr) return 0;
  p[0] = c.icon_seq;
  p[1] = c.chunk_idx;
  p[2] = c.chunk_cnt;
  memcpy(p + 3, c.data, kIconChunkLen);
  return kHeaderLen + kNavIconLen;
}

size_t encodeNavEnd(uint8_t* buf, size_t cap) {
  return begin(Type::NavEnd, 0, buf, cap) == nullptr ? 0 : kHeaderLen;
}

size_t encodeCall(const Call& c, uint8_t* buf, size_t cap) {
  const size_t slen = utf8PrefixLen(c.caller, kMaxCallerLen);
  uint8_t* p = begin(Type::Call, slen, buf, cap);
  if (p == nullptr) return 0;
  memcpy(p, c.caller, slen);
  return kHeaderLen + slen;
}

size_t encodeStatus(const Status& s, uint8_t* buf, size_t cap) {
  uint8_t* p = begin(Type::Status, kStatusFixed, buf, cap);
  if (p == nullptr) return 0;
  p[0] = s.phone_batt_pct;
  wrU32(p + 1, s.unix_time);
  wrU16(p + 5, static_cast<uint16_t>(s.utc_offset_min));
  return kHeaderLen + kStatusFixed;
}

}  // namespace proto
