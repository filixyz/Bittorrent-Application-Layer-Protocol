#include "DynamicBitset.h"
#include <cstdint>
#include <iostream>
#include <bitset>

std::size_t DynamicBitset::get_bytes_for_bits(std::size_t bits) {
  std::size_t extra = bits & (63) ? 1 : 0;
  std::size_t nextr = bits/64;
  return nextr + extra;
}

DynamicBitset::DynamicBitset(std::size_t count) : length(count){
  bitfield.resize(get_bytes_for_bits(count));
  bitfield.shrink_to_fit();
};

DynamicBitset::DynamicBitset(std::span<std::uint8_t> view){
  bitfield.resize(get_bytes_for_bits(view.size()*8));
  std::size_t set_index=0;
  std::uint8_t offset = 0;

  for (auto& byte : view) {
    bitfield[set_index] |= std::uint64_t(byte)<<(8*offset);
    ++offset;
    if (offset % 8 == 0) {
      ++set_index; offset=0;
    }
  }
}

void DynamicBitset::operator|=(const DynamicBitset& other) {
  if (other.length > length) bitfield.resize(other.length);
  for (std::size_t i=0; i < other.bitfield.size(); ++i)
    bitfield[i] |= other.bitfield[i];
}

void DynamicBitset::operator&=(const DynamicBitset& other) {
  if (other.length > length) bitfield.resize(other.length);
  for (std::size_t i=0; i < other.bitfield.size(); ++i)
    bitfield[i] &= other.bitfield[i];
}

DynamicBitset DynamicBitset::operator|(const DynamicBitset& other) {
  DynamicBitset newset = *this;
  newset |= other;
  return newset;
}

DynamicBitset DynamicBitset::operator&(const DynamicBitset& other) {
  DynamicBitset newset = *this;
  newset &= other;
  return newset;
}

void DynamicBitset::print() {
  for (auto& i : bitfield)
    std::cout << std::bitset<64>(i).to_string() << '\n';
}
