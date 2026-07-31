#include "Constants.h"
#include <array>
#include <cstdint>
#include <sys/uio.h>

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
  std::size_t prepare_read(struct iovec*);
  std::size_t prepare_write(struct iovec*);
  void commit_read(std::size_t);
  void commit_write(std::size_t);
  std::size_t r_available();
  std::size_t w_available();
};
