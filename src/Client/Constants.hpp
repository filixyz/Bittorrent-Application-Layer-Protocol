#pragma once
#include <sys/socket.h>
#include <cstddef>

namespace bprotocol::constants {
  inline constexpr char client_id[] = "-JL0001-x4Kn8mR2pL9q";
  inline constexpr std::size_t healthy_peer_count = 50;
  inline constexpr std::size_t tcp_bufexp = 16; // 2^tcp_bufexp: if 16 65,636 bytes as buffer size
  inline constexpr std::size_t hanshake_bufexp = 7; // 128 bytes.
  inline constexpr std::size_t max_inflight_conns = 5;
  inline constexpr std::size_t connection_backlog = 128;
  inline constexpr std::size_t rankify_duration = 10; //secs

  namespace peer {
    constexpr std::size_t max_reties = 3;
    constexpr std::size_t connect_timeout = 10;//seconds;
    constexpr std::size_t retry_timeout = 30;//seconds;
  }
}
