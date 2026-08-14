#include "TCPRingBuffer.h"
#include <cstddef>
#include <sys/uio.h>
#include <utility>

std::size_t tcp_buffer::mask(std::size_t idx) {
  return  idx & (buffer.size()-1);
}

bool tcp_buffer::empty() {
  return prev_action == R && read == write;
}

bool tcp_buffer::full() {
  return prev_action == W && read == write;
}

std::size_t tcp_buffer::size(){
  return write>read ? write-read : buffer.size()-read+write;
}

prepare_t tcp_buffer::prepare_read() {
  prepare_t prepare;
  const std::size_t available = r_available();
  if(read < write || available == 0) {
    prepare.first.first[0].iov_base = &buffer[read];
    prepare.first.first[0].iov_len = available;
    prepare.first.second = 1;
    if (available==0)
      prepare.second = false;
    else
      prepare.second = true;
    return prepare;
  }
  prepare.first.first[0].iov_base = &buffer[read];
  prepare.first.first[0].iov_len = buffer.size() - read;
  prepare.first.first[1].iov_base = &buffer[0];
  prepare.first.first[1].iov_len = write;
  prepare.first.second = 2;
  prepare.second = true;
  return prepare;
}

prepare_t tcp_buffer::prepare_write() {
  prepare_t prepare;
  const std::size_t available = w_available();
  if(write < read || available == 0) {
    prepare.first.first[0].iov_base = &buffer[write];
    prepare.first.first[0].iov_len = available;
    prepare.first.second=1;
    if (available==0)
      prepare.second = false;
    else
      prepare.second = true;
    return prepare;
  }
  prepare.first.first[0].iov_base = &buffer[write];
  prepare.first.first[0].iov_len = buffer.size() - write;
  prepare.first.first[1].iov_base = &buffer[0];
  prepare.first.first[1].iov_len = read;
  prepare.first.second = 2;
  prepare.second = true;
  return prepare;
}

void tcp_buffer::commit_read(std::size_t bytes) {
  read = mask(read+bytes);
  prev_action=R;
}

void tcp_buffer::commit_write(std::size_t bytes) {
  write = mask(write+bytes);
  prev_action=W;
}

std::size_t tcp_buffer::r_available() {
  if(empty()) return 0;
  if(full()) return buffer.size();
  return size();
}

std::size_t tcp_buffer::w_available() {
  if(full()) return 0;
  if(empty()) return buffer.size();
  return buffer.size() - size();
}

void tcp_buffer::reset() {
  prev_action = R;
  read = 0;
  write = 0;
}
