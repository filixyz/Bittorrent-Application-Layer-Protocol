#pragma once
#include <cstddef>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>
#include <bitset>
#include "DynamicBitset.h"

// For interacting with peer manager
struct peer_update {
  enum update_type{ BITFIELD, DOWNUPSPEED, CHOKE_INTEREST_STATE };
  update_type type;
  enum class me_mask { choke=0, interest=1 };
  enum class them_mask { choke=2, interest=3 };
  union message {
    std::pair<std::size_t, std::size_t> down_up_speed;
    std::bitset<4> choke_interest_set {0};
    DynamicBitset bitfield;
  };
};

struct peer_address {
  sockaddr_storage sock_store;
  socklen_t sock_size;
};

// For interacting with file_manager
struct file_manager_update {

};
