#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <queue>
#include <unistd.h>
#include <utility>
#include <vector>
#include <string>
#include <bitset>
#include <ev++.h>
#include <memory>
#include "DynamicBitset.h"
#include "TCPRingBuffer.h"
#include "Randomer.h"
#include "../Errorhandlers/BittorentErrors.h"
#include "ThreadMessageTypes.h"
//Unix Networking Headers here
#include <sys/socket.h>
#include <fcntl.h>
#include <signal.h>
#include <netinet/in.h>
#include <cerrno>
#include <arpa/inet.h>
#include <netdb.h>

class PeerTransferManager;
class PeerConnectionManager;

struct transact {
  bool success;   // if true peer is still connected, otherwise false
  bool bufferred; // if true IO happened with application layer buffer, otherwise false.
};

class PeerConnection {
  enum pstate: std::uint8_t {DISCOVERED, S_HANDSHAKE, C_HANDSHAKE, CONNECTED, DISCONNECTED, DEAD};
  pstate state {DISCOVERED};
  const std::string peer_id{""};
  int socket{};
  std::size_t id;
  std::size_t generation;
  sockaddr_storage store{};
  DynamicBitset bitfield;
  msghdr ephemereal_hdr{};
  // returns: false if error is irrecovereable (disconnected/disconnection candidate)
  // true if error recovereable eg EAGAIN/EWOULDBLOCK/EINTR (connected with issues handled/expected)
  bool handle_errno(int);
  void connect();
  void disconnect();
  friend PeerConnectionManager;
public:
  PeerConnection(std::string, std::size_t);
  transact recv();
  transact send();
  tcp_buffer recv_buffer{};
  tcp_buffer send_buffer{};
  ev::io socket_watcher;
  ev::timer timer_watcher;
};

struct PeerSession {
  PeerConnection* peer {&dummypeer};
  std::size_t id;
  std::size_t generation;
  enum from { me=0, them=1 };
  std::bitset<2> choke {};
  std::bitset<2> interest {};
  std::size_t down_rate{0};
  std::size_t upld_rate{0};
  void set_endpoint(PeerConnection&);
  bool is_dummy();
  void endpoint_disconnected();
private:
  static PeerConnection dummypeer;
};

struct connect_update {
  PeerConnection& peer;
  std::size_t id;
  std::size_t generation;
};

struct disconnect_update {
  PeerConnection& peer;
};

using pconnection_queue = std::queue<connect_update>;
using pdisconnection_queue = std::queue<disconnect_update>;
using pdiscovery_queue = std::queue<peer_address>;
