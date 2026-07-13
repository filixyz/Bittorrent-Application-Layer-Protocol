#ifndef PEER_MANAGER
#define PEER_MANAGER

#include <queue>
#include <vector>
#include <string>
#include <bitset>
#include <ev++.h>
#include <memory>
#include <unordered_map>
#include "Constants.h"
#include "ThreadMessageTypes.h"

class PeerManager {

  using socket_handle_t = int;
  struct PeerAddress;
  struct PeerHandle;
  struct PeerConnection;

  ev::dynamic_loop event_loop;
  ev::timer timer;
  ev::io socket_watcher;
  ev::async queue_consumer_watcher;

  bool seeding {false};
  socket_handle_t client_socket;
  std::queue<std::string> discovered_peers;
  std::queue<peer_update> peer_updates;
  std::unordered_map<std::string, PeerHandle> peer_handles;
  std::vector<PeerConnection> peer_connections;

  void initialize_server_socket();
  void initialize_libev();

  void server_socket_callback();
  void rankify_peers_callback();
  void make_new_connections();

public:
  void run_manager();
};

struct PeerManager::PeerAddress {

};

struct PeerManager::PeerConnection {
  PeerHandle* peer;
  ev::io sock_watch;
};

struct PeerManager::PeerHandle {
  enum from { me=0, them=1 };
  std::string peer_id;
  socket_handle_t socket_fd;
  std::vector<bool> bitfield;
  std::bitset<2> choke{0x2};
  std::bitset<2> interest{0x2};
  std::size_t down_speed{0};
  std::size_t upld_speed{0};
  ev_io socket_watcher;
  ev_timer timer_watcher;
};


#endif
