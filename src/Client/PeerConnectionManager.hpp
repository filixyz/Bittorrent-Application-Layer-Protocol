#ifndef CONNECTION_MANAGER
#define CONNECTION_MANAGER

#include <cstdint>
#include <ev++.h>
#include <queue>
//Unix Networking Headers here
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <cerrno>
#include <arpa/inet.h>
#include <netdb.h>
#include <unordered_map>
#include "ThreadMessageTypes.hpp"
#include "TorrentFile.hpp"
#include "PeerManagerTypes.hpp"
#include "overwritable_cache.hpp"

class pmestablisher_t {
    static constexpr std::size_t handlers_count {3};
    enum spot_t: std::uint8_t {discovered, disconnected, failed};
  private:
    PeerConnectionManager& manager;
    spot_t current {discovered};
    std::array<bool, 3> empties {false};
    std::size_t current_inflight{0};
    ev::async daemon;

    bool discovered_peer_handler();
    bool disconnected_peer_handler();
    bool failed_peer_handler();
    void plus_mask_current(std::size_t spot);
    void round_robin_establisher_scheduler();

  public:
    pmestablisher_t(PeerConnectionManager& __manager);
    void send_notification();
    void single_resolve_notification();
    std::size_t get_current_inflight();
};

class PeerConnectionManager {
  friend class pmestablisher_t;
  template <typename Key>
  using peer_storage_t = std::unordered_map< Key, PeerConnection, peer_manager_hashers>;
private:
  // tells if peers from discovered queue are still actively
  // being tried for connection establishment
  // switch it turned on when discovered populated
  // and turned off when discovered is empty
  bool establishing {false}; // actively establishing connections
  peer_id_gen get_id{};
  ev::dynamic_loop event_loop;
  ev::io server_socket_watcher;

  const TorrentFile& torrent;
  pconnection_queue& connects;
  pdisconnection_queue& disconnects;
  pdiscovery_queue_ipv4& discovered;

  tcp_server_context server;
  peer_storage_t<ipv4_peer_address> ipv4_peers{};
  peer_storage_t<ipv6_peer_address> ipv6_peers{};
  overwritable_cache<ipv4_peer_address, 100> ipv4_discovered_cache;
  std::queue<PeerConnection*> failed_peers;
  std::unordered_map<peer_id_t, PeerConnection*, peer_manager_hashers> peer_ids; // for deduplication after handshake.
  std::size_t connected_peers_count{0};
  pmestablisher_t establisher;

  void ipv6_default_server_sockstore();
  void ipv4_default_server_sockstore();

  void handle_socket_errno(int);
  void handle_ip_errno(int);
  void handle_bind_errno(int);
  bool handle_server_errno(int);
  bool accept_peer_connection();
  void server_socket_callback(ev::io&, int);

  int  initialize_libev();
  void initialize_server_socket();
  void initialize_manager_watchers();

  void initialize_peer(PeerConnection&, peer_key_t&, pipv, psource);
  void server_define_peer(PeerConnection&, int, peer_sock_store_t*);
  void acquire_peer(PeerConnection&);
  void release_peer(PeerConnection&);
  int  parse_handshake(PeerConnection&);
  int  buffer_handshake(PeerConnection&);
  void dispatch_connect(PeerConnection&);
  bool connect(PeerConnection&);
  void erase(PeerConnection&);

  void static peer_socket_callback(ev::io&, int);
  void static peer_timer_callback(ev::timer&, int);
  void drain_discovered();
  void drain_disconnected();
  inline void initiate_new_connection();
  inline void initiate_connection();

public:
  PeerConnectionManager(TorrentFile&, pconnection_queue&, pdisconnection_queue&, pdiscovery_queue_ipv4&);
  void run_manager();
  int get_listening_port();
};

#endif
