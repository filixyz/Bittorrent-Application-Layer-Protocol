#ifndef TRANSFER_MAM
#define TRANSFER_MAN
#include "PeerManagerTypes.h"
#include <ev++.h>

class PeerTransferManager {
  ev::timer rank_timer;
  ev::io event_loop;
  std::vector<PeerSession> peer_connections{};
};

#endif
