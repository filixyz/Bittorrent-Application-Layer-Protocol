#include <array>
#include <cstdint>
#include <ios>
#include <iostream>

class tcp_buffer {
  enum ost { R, W };
  ost prev_action = R;
  std::size_t read{0}, write{0};
  std::array<std::uint8_t, (std::uint64_t(1)<<3 ) > buffer;
  std::size_t mask(std::size_t);
  std::size_t size();

public:
  bool empty();
  bool full();
  tcp_buffer() = default;
  std::uint8_t* get_read_addr();
  std::uint8_t* get_writ_addr();
  void update_read(std::size_t);
  void update_writ(std::size_t);
  std::size_t r_available();
  std::size_t w_available();
};


#include <cstddef>

std::size_t tcp_buffer::mask(std::size_t idx) {  return idx & (buffer.size()-1);  }

bool tcp_buffer::empty() { return prev_action == R && read == write; }

bool tcp_buffer::full() { return prev_action == W && read == write; }

std::size_t tcp_buffer::size(){ if(!full()) return write - read; return 8; }

std::uint8_t* tcp_buffer::get_read_addr() { return &buffer[read]; }

std::uint8_t* tcp_buffer::get_writ_addr() { return &buffer[write]; }

void tcp_buffer::update_read(std::size_t bytes) { read = mask(read+bytes); prev_action=R; }

void tcp_buffer::update_writ(std::size_t bytes) { write = mask(write+bytes); prev_action=W; }

std::size_t tcp_buffer::r_available() {
  if(empty()) return 0;
  return size();
}

std::size_t tcp_buffer::w_available() {
  if(full()) return 0;
  return buffer.size() - size();
}

std::size_t buffer_writer(std::uint8_t* dest, std::size_t len) {
  auto end = dest+len;
  for(; dest != end; ++dest)
    *dest = 0x2D;
  return len;
}


std::size_t buffer_reader(std::uint8_t* dest, std::size_t len) {
  auto end = dest+len;
  for(; dest != end; ++dest)
    *dest = 0x00;
  return len;
}


void print_buffer(std::uint8_t* dest) {
  auto end = dest+8;
  for(; dest != end; ++dest)
    std::cout << (int)*dest << ' ';
  std::cout << '\n';
}

int main() {
  tcp_buffer buffer{};
  auto buffer_head = buffer.get_read_addr();
  std::cout << std::boolalpha;

  std::cout << buffer.r_available() << ' ' << buffer.w_available() << '\n';

  buffer.update_writ(buffer_writer(buffer.get_writ_addr(), buffer.w_available()));
  print_buffer(buffer_head); 
  std::cout << "full, empty: " << buffer.full() << ' ' << buffer.empty() << '\n';
  std::cout << buffer.r_available() << ' ' << buffer.w_available() << '\n';


  buffer.update_read(buffer_reader(buffer.get_read_addr(), buffer.r_available()));
  print_buffer(buffer_head); 
  buffer.update_read(buffer_reader(buffer.get_read_addr(), buffer.r_available()));
  print_buffer(buffer_head); 
  buffer.update_writ(buffer_writer(buffer.get_writ_addr(), (std::size_t)2));
  print_buffer(buffer_head); 
  buffer.update_writ(buffer_writer(buffer.get_writ_addr(), (std::size_t)2));
  print_buffer(buffer_head); 
  std::cout << "full, empty: " << buffer.full() << ' ' << buffer.empty() << '\n';
  std::cout << buffer.r_available() << ' ' << buffer.w_available() << '\n';
  buffer.update_read(buffer_reader(buffer.get_read_addr(), buffer.r_available()));
  print_buffer(buffer_head); 
  std::cout << buffer.r_available() << ' ' << buffer.w_available() << '\n';
}
