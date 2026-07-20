#include "Constants.h"
#include <array>
#include <cstdint>

class tcp_buffer {
  std::size_t read{0}, write{0};
  std::array<std::uint8_t, (std::uint64_t(1)<<client::constants::tcp_bufexp ) > buffer;
  bool empty();
  bool full();
  std::size_t mask(std::size_t);
  std::size_t size();

public:
  tcp_buffer() = default;
  std::uint8_t* get_read_addr();
  std::uint8_t* get_writ_addr();
  void update_read(std::size_t);
  void update_writ(std::size_t);
  std::size_t r_available();
  std::size_t w_available();
};
