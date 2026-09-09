#ifndef OBJECT_POOL
#define OBJECT_POOL

#include <cstddef>
#include <array>
#include <numeric>
#include <vector>
#include <cassert>
#include <bitset>

// Fixed-capacity (N), non-thread-safe object pool intended for exclusive use
// by a single owning thread — no internal synchronization is provided or
// intended.
//
// Recycling philosophy: all N objects are default-constructed once, up
// front, and live for the lifetime of the pool. acquire()/release() only
// hand out and reclaim *slots* — they do not construct, destroy, or reset
// the underlying T. The caller owns initialization of an object after
// acquire() and deinitialization before release(); a slot's contents are
// whatever the previous occupant left behind until the caller sets them.
//
// Trusted-caller contract (unchecked in release builds, assert-only in
// debug): release() must be called at most once per acquire()'d pointer,
// and only with a pointer obtained from this pool's acquire(). Violating
// this corrupts the free list silently outside of debug builds.
//
// Implemented by yours truly, Felix

template <typename T, std::size_t N> class slot_recycling_pool {
  static_assert(N != 0, "Size cannot be zero");
  std::array<T, N> pool;
  std::vector<std::size_t> available_slots;
  std::bitset<N> occupied;
public:

  ~slot_recycling_pool() = default;
  slot_recycling_pool(slot_recycling_pool&&) = delete;
  slot_recycling_pool& operator=(slot_recycling_pool&&) = delete;
  slot_recycling_pool(const slot_recycling_pool&) = delete;
  slot_recycling_pool& operator=(const slot_recycling_pool&) = delete;

  slot_recycling_pool() : pool(), available_slots(N), occupied() {
    std::iota(available_slots.begin(), available_slots.end(), 0);
  }

public:

  struct acquire_t {
    bool acquire_successful;
    T* acquired;
  };

  [[nodiscard]] acquire_t acquire() noexcept {
    if (available_slots.empty())
      return {false, nullptr};

    const std::size_t slot = available_slots.back();
    available_slots.pop_back();
    occupied.set(slot);

    return { true, &pool[slot] };
  }

  void release(T* obj) noexcept {
    std::ptrdiff_t slot = obj - pool.data();
    assert(static_cast<std::size_t>(slot)<N && slot>=0);
    assert(occupied[slot]);
    available_slots.push_back( static_cast<std::size_t>(slot) );
    occupied.reset(slot);
  }

  std::size_t available() const noexcept {
    return available_slots.size();
  }

  std::size_t active() const noexcept {
    return N - available();
  }

};


#endif
