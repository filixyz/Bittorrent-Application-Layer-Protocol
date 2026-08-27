#ifndef OVERWRITABLE_CACHE
#define OVERWRITABLE_CACHE

#include <algorithm>
#include <cstddef>
#include <vector>

template < typename T, std::size_t N > class overwritable_cache {
  static_assert( N != 0 );
  std::vector<T> cache;
  std::size_t write{0};
  std::size_t read{0};
  std::size_t size{0};

  std::size_t plus_mask(std::size_t num) {
    return (num + 1 == N) ? 0 : ++num;
  }
  std::size_t minus_mask(std::size_t num) {
    return (num == 0) ? N - 1 : --num;
  }

public:
  overwritable_cache(): cache(N, T()) {};

  void push (T val) noexcept {
    cache[write] = std::move(val);
    write = plus_mask(write);
    if (size < N)
      ++size;
    else
      read = plus_mask(read);
  }

  bool stale_pop (T& val) noexcept {
    if (isempty())
      return false;
    val = std::move(cache[read]);
    --size;
    read = plus_mask(read);
    return true;
  }

  bool fresh_pop(T& val) noexcept {
    if (isempty())
      return false;
    write = minus_mask(write);
    val = std::move(cache[write]);
    --size;
    return true;
  }
  bool isempty() const noexcept {
    return size == 0;
  }
};

#endif
