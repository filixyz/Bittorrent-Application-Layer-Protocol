#ifndef PEER_MANAGER
#define PEER_MANAGER
#include <string>
#include <bitset>

class PeerManager {
  template <int T> struct Peer;

};

template <int T>
struct PeerManager::Peer {
  std::string peer_id;
  std::string address;
  int port;
  // might concatenate peer_id and port above
  // into an std::vector<std::byte> since this client will
  // be operating in compact mode; first 4 bytes -> address
  //                               last  2 bytes -> port number
  std::bitset<T> bitfield;
  std::bitset<2> choke_flag;
  std::bitset<2> interest_flag;
  std::size_t down_speed{0};
  std::size_t upld_speed{0};
  enum bit_for { client=0, peer=1 };
};

#endif
