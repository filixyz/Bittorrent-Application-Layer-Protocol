#ifndef HASHER
#define HASHER
#include <string>
#include <span>
#include <vector>

struct Hasher{
  static const std::array<std::byte, 20> get_sha1(const std::span<const std::byte>);
  static bool test_buffer_to_sha1(const std::span<const std::byte>, std::string);
  template <std::size_t N>  static std::string hex_stringify_hash(const std::span<std::byte, N>&);
  template <std::size_t N>  static std::string byte_stringify_hash(const std::span<std::byte, N>&);
};

#endif
