#include "DynamicBitset.h"
#include <cstdint>

DynamicBitset::DynamicBitset(std::size_t count) : length(count){
  std::size_t extra = count>64 ? count & (63) : count;
  std::size_t nextr = count/64;
  std::size_t size  = nextr + extra;
  for(std::size_t i = 0; i < size; ++size)
    bitfield[i] = 0;
  bitfield.shrink_to_fit();
};

DynamicBitset::DynamicBitset(std::span<std::uint8_t> view) {
  DynamicBitset newset(view.size());
  std::size_t set_index=0;
  std::size_t byte_count = 0;
  std::uint8_t offset = 0;

  for (auto& byte : view) {
    newset.bitfield[set_index] |= std::uint64_t(byte)<<(8*offset);
    ++offset;
    if (byte_count!=0 && byte_count % 8 == 0) {
      ++set_index; offset=0;
    }
  }
}
