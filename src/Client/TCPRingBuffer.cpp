#include "TCPRingBuffer.h"
#include <cstddef>

std::size_t tcp_buffer::mask(std::size_t idx) { return  idx & (buffer.size()-1); }

bool tcp_buffer::empty() { return read == write; }

bool tcp_buffer::full() { return mask(write + 1) == read; }

std::size_t tcp_buffer::size(){ return write - read; }

std::uint8_t* tcp_buffer::get_read_addr() { return &buffer[read]; }

std::uint8_t* tcp_buffer::get_writ_addr() { return &buffer[write]; }

void tcp_buffer::update_read(std::size_t bytes) { read = mask(read+bytes); }

void tcp_buffer::update_writ(std::size_t bytes) { write = mask(write+bytes); }

std::size_t tcp_buffer::r_available() {
  if(empty()) return 0;
  return size()-1;
}

std::size_t tcp_buffer::w_available() {
  if(full()) return 0;
  return buffer.size() - size() -1;
}
