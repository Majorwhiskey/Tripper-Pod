#include "icon_assembler.h"

#include <string.h>

namespace proto {

bool IconAssembler::feed(const NavIcon& chunk) {
  // decode() already rejects these; checked again so the class is safe standalone.
  if (chunk.chunk_cnt != kIconChunkCount || chunk.chunk_idx >= kIconChunkCount) return false;

  if (!has_seq_ || chunk.icon_seq != seq_) {
    has_seq_ = true;
    seq_ = chunk.icon_seq;
    received_ = 0;
  }
  const bool was_complete = received_ == kAllChunks;
  memcpy(bitmap_ + chunk.chunk_idx * kIconChunkLen, chunk.data, kIconChunkLen);
  received_ |= static_cast<uint8_t>(1u << chunk.chunk_idx);
  return !was_complete && received_ == kAllChunks;
}

bool IconAssembler::readyFor(uint8_t text_icon_seq) const {
  return has_seq_ && received_ == kAllChunks && seq_ == text_icon_seq;
}

void IconAssembler::reset() {
  has_seq_ = false;
  received_ = 0;
}

}  // namespace proto
