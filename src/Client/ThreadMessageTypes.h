#pragma once
#include <cstddef>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>
#include <bitset>
#include "DynamicBitset.h"

// For interacting with peer manager
struct peer_address {
  std::array<std::byte, 18> iport;
  socklen_t ip_size;
};

// For interacting with file_manager
struct verified_piece {
};
