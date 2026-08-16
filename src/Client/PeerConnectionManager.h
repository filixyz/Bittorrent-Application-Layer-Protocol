#ifndef CONNECTION_MANAGER
#define CONNECTION_MANAGER

//Unix Networking Headers here
#include <sys/socket.h>
#include <fcntl.h>
#include <signal.h>
#include <netinet/in.h>
#include <cerrno>
#include <arpa/inet.h>
#include <netdb.h>
#include <unordered_map>
#include "TorrentFile.h"
#include "PeerManagerTypes.h"

class PeerConnectionManager {
  struct server_socket_t
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

  // tells if peers from discovered queue are still actively
  // being tried for connection establishment
  // switch it turned on when discovered populated
  // and turned off when discovered is empty
  bool consuming {false};
  ev::dynamic_loop event_loop;
  ev::io server_socket_watcher;
  const TorrentFile& torrent;
  pconnection_queue& connects;
  pdisconnection_queue& disconnects;
  pdiscovery_queue& discovered;
  server_socket_t server;
  std::unordered_map<std::string, PeerConnection> peer_handles{};
  std::size_t connected_peers_count{0};

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

  void acquire_peer(PeerConnection&);
  void release_peer(PeerConnection&);
  int  parse_handshake(PeerConnection&);
  int  buffer_handshake(PeerConnection&);
  void dispatch_connect(PeerConnection&);
  void handle_disconnect(PeerConnection&);


  void static peer_socket_callback(ev::io&, int);
  // actively initiates connection for disconnected peers and discovered peer addresses
  void establish_connections();
  void reastablish_connections();
  inline void initiate_new_connection();

public:
  PeerConnectionManager(TorrentFile&, pconnection_queue&, pdisconnection_queue&, pdiscovery_queue&);
  void run_manager();
  int get_listening_port();
};

#endif
