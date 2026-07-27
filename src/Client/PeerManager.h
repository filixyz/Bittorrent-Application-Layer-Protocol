#ifndef PEER_MANAGER
#define PEER_MANAGER

#include <cstdint>
#include <queue>
#include <unistd.h>
#include <vector>
#include <string>
#include <bitset>
#include <ev++.h>
#include <memory>
#include <unordered_map>
#include "Constants.h"
#include "ThreadMessageTypes.h"
#include "DynamicBitset.h"
#include "TCPRingBuffer.h"
//--------------- Unix Networking Headers here
#include <sys/socket.h>
#include <fcntl.h>
#include <signal.h>

class PeerManager {
  using socket_handle_t = int;
  struct PeerHandle;
  struct PeerConnection;

  ev::dynamic_loop event_loop;
  ev::timer timer;
  ev::io socket_watcher;
  ev::async queue_consumer_watcher;

  bool seeding {false};
  socket_handle_t client_socket;
  std::queue<peer_address> discovered_peers;
  std::queue<peer_update> peer_updates;
  std::unordered_map<std::string, PeerHandle> peer_handles;
  std::size_t connected_peers_count;
  std::vector<PeerConnection> peer_connections;

  void initialize_server_socket();
  void initialize_libev();
  void server_socket_callback();
  void peer_socket_callback();
  void optimistic_unchoke();
  void rankify_peers_callback();
  void peer_update_callback();
  void discovered_peer_callback();

public:
  void run_manager();
};

struct PeerManager::PeerConnection {
  PeerHandle* peer;
  ev::io sock_watch;
};

struct PeerManager::PeerHandle {
  enum pstate {DISCOVERED, CONNECTING, CONNECTED, DEAD};
  pstate state {DISCOVERED};
  std::string peer_id;
  socket_handle_t socket_fd;
  DynamicBitset bitfield;
  tcp_buffer recv_buffer;
  tcp_buffer send_buffer;
  enum class from: std::uint8_t { me=0, them=1 };
  std::bitset<2> choke{0x2};
  std::bitset<2> interest{0x2};
  std::size_t down_rate{0};
  std::size_t upld_rate{0};
  ev_io socket_watcher;
  ev_timer timer_watcher;
};

#endif
