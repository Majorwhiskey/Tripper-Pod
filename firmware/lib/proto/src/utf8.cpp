#include "utf8.h"

namespace proto {
namespace {

// Length of the valid UTF-8 sequence starting at in[0], or 0 if invalid.
// Rejects overlong forms, surrogates and code points above U+10FFFF.
size_t validSeqLen(const uint8_t* in, size_t avail) {
  const uint8_t b0 = in[0];
  if (b0 >= 0x01 && b0 <= 0x7F) return 1;

  size_t n;
  uint8_t lo = 0x80, hi = 0xBF;  // bounds for the second byte
  if (b0 >= 0xC2 && b0 <= 0xDF) {
    n = 2;
  } else if (b0 >= 0xE0 && b0 <= 0xEF) {
    n = 3;
    if (b0 == 0xE0) lo = 0xA0;
    if (b0 == 0xED) hi = 0x9F;
  } else if (b0 >= 0xF0 && b0 <= 0xF4) {
    n = 4;
    if (b0 == 0xF0) lo = 0x90;
    if (b0 == 0xF4) hi = 0x8F;
  } else {
    return 0;
  }
  if (avail < n) return 0;
  if (in[1] < lo || in[1] > hi) return 0;
  for (size_t i = 2; i < n; ++i) {
    if ((in[i] & 0xC0) != 0x80) return 0;
  }
  return n;
}

}  // namespace

size_t sanitizeUtf8(const uint8_t* in, size_t in_len, char* out, size_t max_bytes) {
  size_t i = 0, o = 0;
  while (i < in_len) {
    size_t n = validSeqLen(in + i, in_len - i);
    if (n == 0) {
      if (o + 1 > max_bytes) break;
      out[o++] = '?';
      ++i;
      continue;
    }
    if (o + n > max_bytes) break;
    for (size_t k = 0; k < n; ++k) out[o++] = static_cast<char>(in[i + k]);
    i += n;
  }
  out[o] = '\0';
  return o;
}

size_t utf8PrefixLen(const char* s, size_t max_bytes) {
  size_t len = 0;
  while (len < max_bytes && s[len] != '\0') ++len;
  if (s[len] == '\0') return len;
  // Cut mid-sequence: back up over continuation bytes to the lead byte.
  while (len > 0 && (static_cast<uint8_t>(s[len]) & 0xC0) == 0x80) --len;
  return len;
}

}  // namespace proto
