#include "text_fit.h"

#include <stdint.h>

namespace app {
namespace {

bool isContinuation(char c) { return (static_cast<uint8_t>(c) & 0xC0) == 0x80; }

size_t nextBoundary(const char* s, size_t len, size_t i) {
  ++i;
  while (i < len && isContinuation(s[i])) ++i;
  return i;
}

}  // namespace

size_t fitPrefix(const char* s, size_t len, int max_px, MeasureFn measure, void* ctx) {
  size_t best = 0;
  for (size_t i = 0; i < len;) {
    const size_t j = nextBoundary(s, len, i);
    if (measure(s, j, ctx) > max_px) break;
    best = j;
    i = j;
  }
  return best;
}

Line wrapLine(const char* s, size_t len, int max_px, MeasureFn measure, void* ctx) {
  size_t start = 0;
  while (start < len && s[start] == ' ') ++start;
  if (start == len) return {len, 0, len};
  const char* t = s + start;
  const size_t n = len - start;

  size_t best = 0;  // end of the last whole word that fits
  for (size_t i = 0; i <= n; ++i) {
    if (i == n || t[i] == ' ') {
      if (measure(t, i, ctx) > max_px) break;
      best = i;
    }
  }
  if (best == 0) {
    best = fitPrefix(t, n, max_px, measure, ctx);
    if (best == 0) best = nextBoundary(t, n, 0);
  }

  size_t next = start + best;
  while (next < len && s[next] == ' ') ++next;
  return {start, best, next};
}

}  // namespace app
