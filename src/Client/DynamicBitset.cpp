#include "DynamicBitset.hpp"
#include <algorithm>
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
  length = view.size()*8;
  bitfield.resize(get_bytes_for_bits(length));
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

bool DynamicBitset::test(std::size_t index) const{
  if (index >= length)
    return false;
  std::size_t field_index = index/64;
  std::size_t bit_index = index & 63;
  return bitfield[field_index] & std::uint64_t(1)<<bit_index;
}

void DynamicBitset::set(std::size_t index){
  if (index >= length)
    return;
  std::size_t field_index = index/64;
  std::size_t bit_index = index & 63;
  bitfield[field_index] |= std::uint64_t(1)<<bit_index;
}

void DynamicBitset::reset(std::size_t index) {
  if (index >= length)
    return;
  std::size_t field_index = index/64;
  std::size_t bit_index = index & 63;
  bitfield[field_index] &= ~(std::uint64_t(1)<<bit_index);
}

void DynamicBitset::reset_set() {
  std::fill(bitfield.begin(), bitfield.end(), std::uint64_t{0});
}
