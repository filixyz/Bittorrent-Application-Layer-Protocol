#pragma once
#include "Constants.hpp"
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <sys/uio.h>

// .iovec holds actual prepare tuple,
// .sucessful tells if I/0 is possible with buffer
// .total_prepared() gives the total bytes prepared for I0
struct prepare_t {
  // .first is the iovec array,
  // .second is the arrays active size
  std::pair<std::array<iovec, 2>, std::size_t> iovec {};
  bool sucessful;
  std::size_t prepared_bytes() {
    return iovec.first[0].iov_len + iovec.first[1].iov_len;
  }
};

template <std::size_t N> class tcp_buffer {
  static_assert(N!=0, "tcp_buffer size cannot be zero");
  enum ost { R, W };
  ost prev_action = R;
  std::size_t read{0}, write{0};
  std::array<std::uint8_t, N> buffer;
  std::size_t mask(std::size_t);
  std::size_t size();

public:
  bool empty();
  bool full();
  tcp_buffer() = default;
  prepare_t prepare_read();
  prepare_t prepare_write();
  void commit_read(std::size_t);
  void commit_write(std::size_t);
  std::size_t r_available();
  std::size_t w_available();
  void reset();
};

template <std::size_t N> std::size_t tcp_buffer<N>::mask(std::size_t idx) {
  if constexpr (std::has_single_bit(N)) {
    return idx & (N-1);
  } else {
    return idx % N;
  }
}

template <std::size_t N> bool tcp_buffer<N>::empty() {
  return prev_action == R && read == write;
}

template <std::size_t N> bool tcp_buffer<N>::full() {
  return prev_action == W && read == write;
}

template <std::size_t N> std::size_t tcp_buffer<N>::size(){
  return write>read ? write-read : buffer.size()-read+write;
}

template <std::size_t N> prepare_t tcp_buffer<N>::prepare_read() {
  prepare_t prepare;
  const std::size_t available = r_available();
  if(read < write || available == 0) {
    prepare.iovec.first[0].iov_base = &buffer[read];
    prepare.iovec.first[0].iov_len = available;
    prepare.iovec.second = 1;
    if (available==0)
      prepare.sucessful = false;
    else
      prepare.sucessful = true;
    return prepare;
  }
  prepare.iovec.first[0].iov_base = &buffer[read];
  prepare.iovec.first[0].iov_len = buffer.size() - read;
  prepare.iovec.first[1].iov_base = &buffer[0];
  prepare.iovec.first[1].iov_len = write;
  prepare.iovec.second = 2;
  prepare.sucessful = true;
  return prepare;
}

template <std::size_t N> prepare_t tcp_buffer<N>::prepare_write() {
  prepare_t prepare;
  const std::size_t available = w_available();
  if(write < read || available == 0) {
    prepare.iovec.first[0].iov_base = &buffer[write];
    prepare.iovec.first[0].iov_len = available;
    prepare.iovec.second=1;
    if (available==0)
      prepare.sucessful = false;
    else
      prepare.sucessful = true;
    return prepare;
  }
  prepare.iovec.first[0].iov_base = &buffer[write];
  prepare.iovec.first[0].iov_len = buffer.size() - write;
  prepare.iovec.first[1].iov_base = &buffer[0];
  prepare.iovec.first[1].iov_len = read;
  prepare.iovec.second = 2;
  prepare.sucessful = true;
  return prepare;
}

template <std::size_t N> void tcp_buffer<N>::commit_read(std::size_t bytes) {
  if (bytes == 0) return;
  read = mask(read+bytes);
  prev_action=R;
}

template <std::size_t N> void tcp_buffer<N>::commit_write(std::size_t bytes) {
  if (bytes == 0) return;
  write = mask(write+bytes);
  prev_action=W;
}

template <std::size_t N> std::size_t tcp_buffer<N>::r_available() {
  if(empty()) return 0;
  if(full()) return buffer.size();
  return size();
}

template <std::size_t N> std::size_t tcp_buffer<N>::w_available() {
  if(full()) return 0;
  if(empty()) return buffer.size();
  return buffer.size() - size();
}

template <std::size_t N> void tcp_buffer<N>::reset() {
  prev_action = R;
  read = 0;
  write = 0;
}

using hanshake_buffer = tcp_buffer<bprotocol::constants::hanshake_bufexp>;
using session_buffer = tcp_buffer<bprotocol::constants::tcp_bufexp>;
