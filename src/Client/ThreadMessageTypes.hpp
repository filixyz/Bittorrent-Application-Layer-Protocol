#pragma once
#include "spsc_queue.hpp"
#include <ev++.h>
#include <array>
// For interacting with peer manager

template<class T, std::size_t N>
struct nspsc_queue {
  spsc_queue<T, N> queue;
  ev::async consumer;
};

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
