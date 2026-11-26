#pragma once

#include <stddef.h>
#include <stdint.h>

namespace proto {

// Copies at most `max_bytes` bytes of `in` into `out` and NUL-terminates it, so
// `out` must hold max_bytes + 1. Truncation never splits a code point. Each byte
// that does not start a valid UTF-8 sequence (including NUL, which would cut the
// C string short) becomes '?'. Returns the number of bytes written before the NUL.
size_t sanitizeUtf8(const uint8_t* in, size_t in_len, char* out, size_t max_bytes);

// Length in bytes of the longest prefix of `s` that is at most `max_bytes` long and
// ends on a code-point boundary. Assumes `s` is valid UTF-8.
size_t utf8PrefixLen(const char* s, size_t max_bytes);

}  // namespace proto
