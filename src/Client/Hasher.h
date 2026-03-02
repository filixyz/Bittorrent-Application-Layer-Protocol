#ifndef HASHER
#define HASHER
#include <string>
#include <span>
#include <vector>

struct Hasher{
  static const std::vector<std::byte> get_sha1(const std::span<const std::byte>);
  static bool test_buffer_to_sha1(const std::span<const std::byte>, std::string);
  static std::string hex_stringify_hash(const std::vector<std::byte>&);
};

#endif
