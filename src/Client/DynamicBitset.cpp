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
  std::uint8_t offset = 0;

  for (auto& byte : view) {
    newset.bitfield[set_index] |= std::uint64_t(byte)<<(8*offset);
    ++offset;
    if (offset % 8 == 0) {
      ++set_index; offset=0;
    }
  }
}

DynamicBitset DynamicBitset::operator|(const DynamicBitset& other) {
  bool is_other_longest = other.length > length;
  const DynamicBitset& longest = is_other_longest ? other : *this;
  const DynamicBitset& shortest = is_other_longest ? *this: other;
  DynamicBitset newset = longest;

  for (std::size_t i=0; i < shortest.bitfield.size(); ++i)
    newset.bitfield[i] |= shortest.bitfield[i];

  return newset;
}

DynamicBitset DynamicBitset::operator&(const DynamicBitset& other) {
  bool is_other_longest = other.length > length;
  std::size_t newsetl = is_other_longest ? other.length : length;
  const DynamicBitset& shortest = is_other_longest ? *this : other;
  DynamicBitset newset (newsetl);

  for (std::size_t i=0; i < shortest.bitfield.size(); ++i)
    newset.bitfield[i] &= shortest.bitfield[i];

  return newset;
}
