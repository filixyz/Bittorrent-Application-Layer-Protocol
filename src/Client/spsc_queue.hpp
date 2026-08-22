#ifndef SPSC_TEMPLATE_QUEUE
#define SPSC_TEMPLATE_QUEUE

// Inspired by rigtorp optimized lockfree interthreaded ringbuffer.

#include <vector>
#include <atomic>

template<typename Type, std::size_t Size> class spsc_queue {
  static constexpr std::size_t dummy_slot_count{1};
  static constexpr std::size_t cacheline{64};
  std::vector<Type> queue{};
  alignas(cacheline) std::atomic<std::size_t> read_idx_{0};
  alignas(cacheline) std::size_t cached_read_idx_{0};
  alignas(cacheline) std::atomic<std::size_t> write_idx_{0};
  alignas(cacheline) std::size_t cached_write_idx_{0};
public:
  spsc_queue(): queue(Size + dummy_slot_count, Type()) {}
  spsc_queue (const spsc_queue&) = delete;
  spsc_queue& operator=(const spsc_queue&) = delete;

  [[nodiscard]] bool push(Type val) {
    auto const write_index = write_idx_.load(std::memory_order_relaxed);
    auto next_write_index = write_index + 1;
    if (next_write_index == queue.size())
      next_write_index = 0;
    if ( next_write_index == cached_read_idx_ ) {
      cached_read_idx_ = read_idx_.load(std::memory_order_acquire);
      if ( next_write_index == cached_read_idx_ )
        return false; // queue full
    }
    queue[write_index] = std::move(val);
    write_idx_.store(next_write_index, std::memory_order_release);
    return true;
  }

  [[nodiscard]] bool pop(Type& val) {
    auto const read_index = read_idx_.load(std::memory_order_relaxed);
    if (read_index == cached_write_idx_) {
      cached_write_idx_ = write_idx_.load(std::memory_order_acquire);
      if (read_index == cached_write_idx_)
        return false; // queue empty
    }
    val = std::move(queue[read_index]);
    auto next_read_index = read_index+1;
    if (next_read_index == queue.size())
      next_read_index = 0;
    read_idx_.store(next_read_index, std::memory_order_release);
    return true;
  }
};

#endif
