// Text formatting for the display. All functions NUL-terminate `out` and never
// write more than `cap` bytes; output is truncated if `cap` is too small.
#pragma once

#include <stddef.h>
#include <stdint.h>

namespace app {

// "350 m", "1.2 km", "12 km"; "--" for proto::kUnknownU32.
void formatDistance(uint32_t metres, char* out, size_t cap);

// "12 min", "1 h 05 min"; "--" for proto::kUnknownU16.
void formatDuration(uint16_t minutes, char* out, size_t cap);

// "HH:MM" 24-hour from seconds since local midnight; "--:--" if negative (unknown).
void formatClock(int32_t sec_of_day, char* out, size_t cap);

}  // namespace app
