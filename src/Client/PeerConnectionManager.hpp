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
#include "bittorrent_messages.hpp"
#include "slot_pool.hpp"

using connection_pool_t = slot_recycling_pool<PeerConnection, 200>;
using connection_slot =  connection_pool_t::pool_slot;
using handshake_t = bittorrent_messages::handshake_t;
template <typename Key>
using peer_register_t = std::unordered_map< Key, connection_slot*, peer_manager_hashers>;

struct tcp_server_context
{
  int socket{};
  union {sockaddr_in ipv4; sockaddr_in6 ipv6;} store{};
  socklen_t store_len{};
  int flags {SOCK_STREAM|SOCK_NONBLOCK};
  int trspt_proto{IPPROTO_TCP};
  int off_ipv6only{0};
  bool ipv4_support{false};
  int port{0};
};

struct peer_failure_update {
  connection_slot* connection;
  std::size_t cached_generation;
};

class connection_statistics_t {
  std::size_t connected_bittorrent_peers;
  std::size_t inbound_inflight;
  std::size_t outbound_inflight;
public:
  bool has_met_connection_quota();

  std::size_t get_bittorrent_connected();
  std::size_t get_inbound_inflight();
  std::size_t get_outbound_inflight();

  void increment_outbound_inflight();
  void increment_inbound_inflight();
  void single_inbound_resolved();
  void single_outbound_resolved();
  void increment_connected_bittorrent_peers();
  void decrement_connected_bittorrent_peers();
};

class inbound_scheduler_t {
    static constexpr std::size_t handlers_count {3};
    enum spot_t: std::uint8_t {discovered, disconnected, failed};
  private:
    PeerConnectionManager& manager;
    spot_t current {discovered};
    std::array<bool, handlers_count> empties {false};
    ev::async daemon;

    bool discovered_peer_handler();
    bool disconnected_peer_handler();
    bool failed_peer_handler();
    void plus_mask_current(std::size_t spot);
    void round_robin_establisher_scheduler();
    bool initiate_connect(PeerConnection&);

  public:
    inbound_scheduler_t(PeerConnectionManager& __manager);
    void send_notification();
};

struct outbound_server_t {
  tcp_server_context parameters;
  ev::io watcher;
};

class PeerConnectionManager { friend class inbound_scheduler_t;

  ev::dynamic_loop event_loop;

  connection_statistics_t statistics;
  connection_pool_t connection_pool;
  peer_register_t<ipv4_peer_address> ipv4_peers{};
  peer_register_t<ipv6_peer_address> ipv6_peers{};

  outbound_server_t outbound_connection_server;

  pdisconnection_queue disconnects;
  pdiscovery_queue_ipv4 discovered; //-------------------------------------->  need to coalesce these two into 1 object
  overwritable_cache<ipv4_peer_address, 100> cached_discoveries_ipv4; //---->  changed my mind on this; will be pain to implement
  std::queue<peer_failure_update> failed_peers;
  inbound_scheduler_t inbound_connection_scheduler;

  const TorrentFile& torrent;
  const handshake_t handshake;
  pconnection_queue& connects;

  void ipv6_default_server_sockstore();
  void ipv4_default_server_sockstore();
  int  initialize_libev();
  void initialize_server_socket();
  void initialize_manager_watchers();
  handshake_t compute_handshake();

  void handle_socket_errno(int);
  void handle_ip_errno(int);
  void handle_bind_errno(int);
  bool handle_server_errno(int);
  void server_socket_callback(ev::io&, int);

  bool accept_peer_connection();

  void initialize_server_specifics(PeerConnection&, int, peer_sock_store_t*);
  bool connect(PeerConnection&);
  bool peer_transport_level_connected(PeerConnection&);
  void deregister_from_map(PeerConnection&);
  void delete_peer_connection(PeerConnection&);

  void handle_peer_failure(PeerConnection&);
  void handle_peer_application_level_handshake(PeerConnection&, int event);
  void handle_peer_transport_level_initiations(PeerConnection&, int event);
  void handle_peer_connection_and_dispatch(PeerConnection&);

  void arm_connection_watchers(PeerConnection&, int event);
  void modify_peer_socket_w_event(PeerConnection&, int event);
  void stop_connection_watchers(PeerConnection&);

  void static peer_socket_callback(ev::io&, int);
  void static peer_timer_callback(ev::timer&, int);

  void drain_discovered();
  void notify_disconnected();

public:
  PeerConnectionManager(TorrentFile&, pconnection_queue&);
  void run_manager();
  int get_listening_port();
};

#endif
