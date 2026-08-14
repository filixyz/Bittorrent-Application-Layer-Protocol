#pragma once
#include "Constants.h"
#include <array>
#include <cstdint>
#include <sys/uio.h>

// .first holds actual prepare tuple,
// .second tells if I/0 is possible with buffer
using prepare_t = std::pair<std::pair<iovec[2], std::size_t>, bool>;

class tcp_buffer {
  enum ost { R, W };

  ost prev_action = R;
  std::size_t read{0}, write{0};
  std::array<std::uint8_t, (std::uint64_t(1)<<bprotocol::constants::tcp_bufexp ) > buffer;
  bool empty();
  bool full();
  std::size_t mask(std::size_t);
  std::size_t size();

public:
  tcp_buffer() = default;
  prepare_t prepare_read();
  prepare_t prepare_write();
  void commit_read(std::size_t);
  void commit_write(std::size_t);
  std::size_t r_available();
  std::size_t w_available();
  void reset();
};
