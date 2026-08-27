#include "../Client/overwritable_cache.hpp"
#include <iostream>
int main() {
  overwritable_cache<int, 20> cache{};
  int count = 0;
  while (count != 40)
    cache.push(count++);
  while (count != 27) {
    int value{};
    cache.fresh_pop(value);
    std::cout << value << '\n';
    --count;
  }
  while (count != 36) {
    int value{};
    cache.stale_pop(value);
    std::cout << value << '\n';
    ++count;
  }
}
