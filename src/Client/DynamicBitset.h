#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
#include <span>

class DynamicBitset {
  std::vector<std::uint64_t> bitfield;
  std::size_t length{};
  std::size_t get_bytes_for_bits(std::size_t);
public:
  DynamicBitset()=default;
  DynamicBitset(std::size_t count);
  DynamicBitset(std::span<std::uint8_t> bitview);
  DynamicBitset operator&(const DynamicBitset&);
  void operator&=(const DynamicBitset&);
  DynamicBitset operator|(const DynamicBitset&);
  void operator|=(const DynamicBitset&);
  // if you set, test, or reset an index>=bitfield.length
  // no bitwise operation will be performed.
  bool test(std::size_t) const;
  void set(std::size_t);
  void reset(std::size_t);
  void print();
};
