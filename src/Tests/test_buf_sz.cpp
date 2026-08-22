#include <array>
#include <iostream>
#include <cstdint>
int main() {
  std::array<std::uint8_t, std::uint64_t(1)<<16> test;
  std::cout << test.size() << '\n';
}
