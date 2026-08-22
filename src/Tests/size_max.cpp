
#include <cmath>
#include <stddef.h>
#include <iostream>
#include <limits>

int main() {
  std::cout << std::numeric_limits<std::size_t>::max() << " is the max for std::size_t\n";
  std::cout << std::numeric_limits<size_t>::max() << " is the max for c size_t\n";
  std::cout << std::numeric_limits<unsigned long long>::max() << " is the max of unsigned long long\n";
  std::cout << sizeof(std::size_t) << " bytes is the sizeof( std::size_t )\n";
}
