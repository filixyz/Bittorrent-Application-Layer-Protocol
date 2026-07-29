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
#include "Randomer.h"
#include "../Errorhandlers/BittorentErrors.h"
//Unix Networking Headers here
#include <sys/socket.h>
#include <fcntl.h>
#include <signal.h>
#include <netinet/in.h>
#include <cerrno>

class PeerManager {
  using socket_handle_t = int;
  struct PeerHandle;
  struct PeerConnection;
  struct server_sock_store
  {
    int socket{};
    sockaddr_storage store{};
    socklen_t store_len{};
    int flags {SOCK_STREAM|SOCK_NONBLOCK};
    int trspt_proto{IPPROTO_TCP};
    int off_ipv6only{0};
    bool ipv4_support{false};
  };

  ev::dynamic_loop event_loop;
  ev::timer timer;
  ev::io socket_watcher;
  ev::async queue_consumer_watcher;

  bool seeding {false};
  bool ppool_cascade_draining{false};
  server_sock_store server;
  std::queue<peer_address> discovered_peers;
  std::queue<peer_update> peer_updates;
  std::unordered_map<std::string, PeerHandle> peer_handles;
  std::size_t connected_peers_count;
  std::vector<PeerConnection> peer_connections;

  void ipv6_default_server_sockstore();
  void ipv4_default_server_sockstore();

  void handle_socket_errno(int);
  void handle_ip_errno(int);
  void handle_bind_errno(int);
  void handle_listen_errno(int);
  void handle_connect_errno(int);
  void handle_peer_errno(int);

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
  enum pstate {DISCOVERED, CONNECTING, CONNECTED, DISCONNECTED, DEAD};
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
