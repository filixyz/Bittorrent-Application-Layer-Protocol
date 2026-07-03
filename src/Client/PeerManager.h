#ifndef PEER_MANAGER
#define PEER_MANAGER
#include <queue>
#include <string>
#include <bitset>
#include <set>
#include <ev++.h>

class PeerManager {

  struct Peer;
  class peer_priority_queue;
  class peer_priotity_heap;

  bool seeding {false};
  std::queue<std::string> incoming_meta_peers;
  std::set<std::string> meta_peers;
  std::vector<Peer> peer_connections;  // collections of successfully connected peers
};

struct PeerManager::Peer {
  enum bit_for { client=0, peer=1 };
  std::string peer_id;
  std::string address_and_port{6};

  std::bitset<2> choke_flag;
  std::bitset<2> interest_flag;
  std::size_t down_speed{0};
  std::size_t upld_speed{0};

  ev::io sock_watch;      // to listen for piece requests and bitfield notifications;
  ev::timer keep_alive;   // for periodiic keep_alives
};

class PeerManager::peer_priority_queue {

  struct peer_compare {
    bool& seeding;
    constexpr bool operator() (const Peer& lhs, const Peer& rhs) {
      return seeding ? lhs.down_speed > rhs.down_speed : lhs.upld_speed > rhs.upld_speed;
    }
  };

  std::priority_queue<Peer, std::vector<Peer>, peer_compare> pr;
};

#endif
