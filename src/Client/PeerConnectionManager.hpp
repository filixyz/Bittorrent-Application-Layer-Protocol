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
//static_assert(std::is_standard_layout_v<pool_slot>);
template <typename Key> using peer_storage_t = std::unordered_map< Key, connection_slot*, peer_manager_hashers>;

struct peer_failure_update {
  connection_slot* connection;
  std::size_t cached_generation;
};

class pmestablisher_t {
    static constexpr std::size_t handlers_count {3};
    enum spot_t: std::uint8_t {discovered, disconnected, failed};
  private:
    PeerConnectionManager& manager;
    spot_t current {discovered};
    std::array<bool, handlers_count> empties {false};
    std::size_t current_inflight{0};
    std::size_t connected_bittorrent_peers_count{0};
    bool establishing {false};
    ev::async daemon;

    bool discovered_peer_handler();
    bool disconnected_peer_handler();
    bool failed_peer_handler();
    void plus_mask_current(std::size_t spot);
    void round_robin_establisher_scheduler();
    bool initiate_connect(PeerConnection&);

  public:
    pmestablisher_t(PeerConnectionManager& __manager);
    std::size_t get_current_inflight();
    bool has_met_connection_qouta();
    void send_notification();
    void single_resolve_notification();
    void increment_connected_bittorrent_peers();
    void decrement_connected_bittorrent_peers();
};


class PeerConnectionManager {  friend class pmestablisher_t;

  peer_id_gen get_id{};

  ev::dynamic_loop event_loop;
  ev::io server_socket_watcher;

  const TorrentFile& torrent;
  pconnection_queue& connects;
  pdisconnection_queue& disconnects;
  pdiscovery_queue_ipv4& discovered;
  const bittorrent_messages::handshake_t handshake;
  pmestablisher_t establisher;
  tcp_server_context server;

  connection_pool_t connection_pool;
  peer_storage_t<ipv4_peer_address> ipv4_peers{};
  peer_storage_t<ipv6_peer_address> ipv6_peers{};
  overwritable_cache<ipv4_peer_address, 100> ipv4_discovered_cache;
  std::queue<peer_failure_update> failed_peers;
  //std::unordered_map<peer_id_t, PeerConnection*, peer_manager_hashers> peer_ids; // for deduplication after handshake.

  void ipv6_default_server_sockstore();
  void ipv4_default_server_sockstore();
  int  initialize_libev();
  void initialize_server_socket();
  void initialize_manager_watchers();
  bittorrent_messages::handshake_t compute_handshake();

  void handle_socket_errno(int);
  void handle_ip_errno(int);
  void handle_bind_errno(int);
  bool handle_server_errno(int);
  void server_socket_callback(ev::io&, int);

  bool accept_peer_connection();

  void initialize_server_specifics(PeerConnection&, int, peer_sock_store_t*);
  void dispatch_connect(PeerConnection&);
  bool connect(PeerConnection&);
  void deregister_from_map(PeerConnection&);
  void handle_peer_failure(PeerConnection&);
  void delete_peer_connection(PeerConnection&);
  void handle_peer_application_level_handshake(PeerConnection&, int event);
  void handle_peer_transport_level_initiations(PeerConnection&, int event);
  void arm_connection_watchers(PeerConnection&, int event, bool reset_timer);
  void stop_connection_watchers(PeerConnection&);
  bool peer_transport_level_connected(PeerConnection&);

  void static peer_socket_callback(ev::io&, int);
  void static peer_timer_callback(ev::timer&, int);

  void drain_discovered();
  void drain_disconnected();

public:
  PeerConnectionManager(TorrentFile&, pconnection_queue&, pdisconnection_queue&, pdiscovery_queue_ipv4&);
  void run_manager();
  int get_listening_port();
};

#endif
