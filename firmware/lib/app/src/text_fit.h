// Fitting UTF-8 text into pixel widths. The width measure is injected so this is
// host-testable without a display.
#pragma once

#include <stddef.h>

namespace app {

// Pixel width of the first `len` bytes of `s`.
using MeasureFn = int (*)(const char* s, size_t len, void* ctx);

// Longest prefix of s[0, len) that ends on a code-point boundary and measures
// <= max_px. May return 0.
size_t fitPrefix(const char* s, size_t len, int max_px, MeasureFn measure, void* ctx);

struct Line {
  size_t start;  // offset of the first byte to draw (leading spaces skipped)
  size_t len;    // bytes to draw from `start`
  size_t next;   // offset where the following line's text begins
};

// Next line of s[0, len) for word wrap: breaks after the last whole word that fits,
// or mid-word if a single word is wider than max_px. Always consumes at least one
// code point while text remains, so callers cannot loop forever.
Line wrapLine(const char* s, size_t len, int max_px, MeasureFn measure, void* ctx);

}  // namespace app
