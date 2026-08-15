#pragma once
#include <cstddef>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>
#include <bitset>
#include "DynamicBitset.h"

// For interacting with peer manager
struct peer_address {
  sockaddr_storage sock_store;
  socklen_t sock_size;
};

// For interacting with file_manager
struct file_manager_update {

};
