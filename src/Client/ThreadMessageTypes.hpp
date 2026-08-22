#pragma once

#include <array>
// For interacting with peer manager
struct ipv4_peer_address {
  std::array<std::byte, 6> iport{};
  bool operator==(const ipv4_peer_address&) const = default;
};
struct ipv6_peer_address {
  std::array<std::byte, 18> iport{};
  bool operator==(const ipv6_peer_address&) const = default;
};
// For interacting with file_manager
struct verified_piece {
};
