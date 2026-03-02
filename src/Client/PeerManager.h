#ifndef PEER_MANAGER
#define PEER_MANAGER
#include <string>
#include <bitset>

enum {
  my_flag=0,
  peer_flag=1
};

template <int T>
struct Peer{
  std::string peer_id;
  std::string address;
  unsigned port;
  std::bitset<T> bitfield;
  std::bitset<2> choke_flag;
  std::bitset<2> interest_flag;
  unsigned down_speed;
  unsigned upld_speed;
};
class PeerManager {

};
#endif
