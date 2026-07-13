#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
#include <span>

class DynamicBitset {
  std::vector<std::uint64_t> bitfield;
  std::size_t length;
public:
  DynamicBitset(std::size_t count);
  DynamicBitset(std::span<std::uint8_t> bitview);
  DynamicBitset operator&(const DynamicBitset&);
  DynamicBitset operator|(const DynamicBitset&);
  bool test(std::size_t) const;
  void set(std::size_t);
  void reset(std::size_t);
};
