#pragma once
#include <cstddef>
#include <utility>
#include <bitset>
#include "DynamicBitset.h"

// For interacting with peer manager
struct peer_update {
  enum update_type{ BITFIELD, DOWNUPSPEED, CHOKE_INTEREST_STATE };
  enum class me_mask { choke=0, interest=1 };
  enum class them_mask { choke=2, interest=3 };
  update_type type;
  union message {
    std::pair<std::size_t, std::size_t> down_up_speed;
    std::bitset<4> choke_interest_set {0};
    DynamicBitset bitfield;
  };
};

struct file_manager_update {

};
