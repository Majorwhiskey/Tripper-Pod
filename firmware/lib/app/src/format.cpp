#include "format.h"

#include <stdio.h>

#include "proto.h"

namespace app {

void formatDistance(uint32_t m, char* out, size_t cap) {
  if (m == proto::kUnknownU32) {
    snprintf(out, cap, "--");
  } else if (m < 995) {
    // 995..999 would round to "1000 m"; switch to km there instead.
    snprintf(out, cap, "%lu m", static_cast<unsigned long>((m + 5) / 10 * 10));
  } else if (m < 9950) {
    const unsigned long tenths = (m + 50) / 100;
    snprintf(out, cap, "%lu.%lu km", tenths / 10, tenths % 10);
  } else {
    // m + 500 would overflow near UINT32_MAX.
    snprintf(out, cap, "%lu km", static_cast<unsigned long>(m / 1000 + (m % 1000 >= 500)));
  }
}

void formatDuration(uint16_t min, char* out, size_t cap) {
  if (min == proto::kUnknownU16) {
    snprintf(out, cap, "--");
  } else if (min < 60) {
    snprintf(out, cap, "%u min", static_cast<unsigned>(min));
  } else {
    snprintf(out, cap, "%u h %02u min", static_cast<unsigned>(min / 60),
             static_cast<unsigned>(min % 60));
  }
}

void formatClock(int32_t s, char* out, size_t cap) {
  if (s < 0) {
    snprintf(out, cap, "--:--");
  } else {
    s %= 86400;
    snprintf(out, cap, "%02ld:%02ld", static_cast<long>(s / 3600), static_cast<long>(s / 60 % 60));
  }
}

}  // namespace app
