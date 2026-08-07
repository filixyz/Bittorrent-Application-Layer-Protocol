#ifndef PEER_MANAGER
#define PEER_MANAGER

#include <cstddef>
#include <cstdint>
#include <mutex>
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
#include "TorrentFile.h"
//Unix Networking Headers here
#include <sys/socket.h>
#include <fcntl.h>
#include <signal.h>
#include <netinet/in.h>
#include <cerrno>
#include <arpa/inet.h>
#include <netdb.h>

class PeerManager {
  struct server_sock_store
  {
    int socket{};
    sockaddr_storage store{};
    socklen_t store_len{};
    int flags {SOCK_STREAM|SOCK_NONBLOCK};
    int trspt_proto{IPPROTO_TCP};
    int off_ipv6only{0};
    bool ipv4_support{false};
    int port{0};
  };
  struct PeerHandle
  {
    enum pstate {DISCOVERED, CONNECTING, CONNECTED, DISCONNECTED, DEAD};
    pstate state {DISCOVERED};
    const std::string peer_id{""};
    int socket{};
    sockaddr_storage store{};
    DynamicBitset bitfield;
    tcp_buffer recv_buffer;
    tcp_buffer send_buffer;
    enum class from: std::uint8_t { me=0, them=1 };
    std::bitset<2> choke{0x2};
    std::bitset<2> interest{0x2};
    std::size_t down_rate{0};
    std::size_t upld_rate{0};
    ev::io socket_watcher;
    ev::timer timer_watcher;
    PeerHandle(std::string, std::size_t);
  };
  struct PeerConnection{
    PeerHandle* peer {&nullpeer};
    void set_peer(PeerHandle&);
    void reset();
  private:
    static PeerHandle nullpeer; // dummy placeholder peer
  };

  ev::dynamic_loop event_loop;
  ev::timer peer_pool_timer;
  ev::io server_socket_watcher;
  ev::async queue_consumer_watcher;
  ev::async peer_update_watcher;

  const TorrentFile& torrent;

  bool seeding {false};
  bool ppool_cascade_draining{false};
  server_sock_store server;
  std::queue<peer_address> discovered_peers{};
  std::queue<peer_update> peer_updates{};
  std::unordered_map<std::string, PeerHandle> peer_handles{};
  std::size_t connected_peers_count{0};
  std::mutex peer_pool_mutex;
  std::vector<PeerConnection> peer_connections{};

  void ipv6_default_server_sockstore();
  void ipv4_default_server_sockstore();

  void handle_socket_errno(int);
  void handle_ip_errno(int);
  void handle_bind_errno(int);
  void handle_peer_errno(int);

  int  initialize_libev();
  void initialize_server_socket();
  void initialize_manager_watchers();
  void initialize_peer_pool();

  void add_connected_peer(PeerHandle&);
  bool handle_server_errno(int);
  bool accept_peer_connection();
  void server_socket_callback(ev::io&, int);

  void peer_socket_callback();
  void optimistic_unchoke();
  void rankify_peers_callback();
  void peer_update_callback();
  void discovered_peer_callback();

public:
  PeerManager(TorrentFile&);
  void run_manager();
  int get_listening_port();
};

#endif
