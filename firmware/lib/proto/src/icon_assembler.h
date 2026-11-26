#pragma once

#include <stdint.h>

#include "proto.h"

namespace proto {

// Collects the NAV_ICON chunks of one icon_seq. A chunk with a different seq
// discards whatever was collected, so a half-received icon is never shown.
class IconAssembler {
 public:
  // Returns true when this chunk completes the icon.
  bool feed(const NavIcon& chunk);

  // True only for a complete icon whose seq matches the latest NAV_TEXT.
  bool readyFor(uint8_t text_icon_seq) const;

  // 64x64 1-bpp bitmap; valid only when readyFor() is true.
  const uint8_t* bitmap() const { return bitmap_; }

  void reset();

 private:
  static constexpr uint8_t kAllChunks = (1u << kIconChunkCount) - 1;

  bool has_seq_ = false;
  uint8_t seq_ = 0;
  uint8_t received_ = 0;  // bit i set = chunk i stored
  uint8_t bitmap_[kIconBytes] = {};
};

}  // namespace proto
