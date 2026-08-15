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

  ev::dynamic_loop event_loop;
  ev::io server_socket_watcher;
  ev::async queue_consumer_watcher;
  ev::async peer_update_watcher;

  const TorrentFile& torrent;
  pconnection_queue& connects;
  pdisconnection_queue disconnects;

  bool ppool_cascade_draining{false};
  server_socket_t server;
  std::unordered_map<std::string, PeerConnection> peer_handles{};
  std::size_t connected_peers_count{0};

  void ipv6_default_server_sockstore();
  void ipv4_default_server_sockstore();

  void handle_socket_errno(int);
  void handle_ip_errno(int);
  void handle_bind_errno(int);

  int  initialize_libev();
  void initialize_server_socket();
  void initialize_manager_watchers();
  void initialize_peer_pool();

  void acquire_peer(PeerConnection&);
  void release_peer(PeerConnection&);

  int parse_handshake(PeerConnection&);
  int buffer_handshake(PeerConnection&);

  void add_connected_peer(PeerConnection&);
  bool handle_server_errno(int);
  bool accept_peer_connection();
  void server_socket_callback(ev::io&, int);

  void static handle_peer_errno(int, PeerConnection&);
  void static peer_socket_callback(ev::io&, int);

  void peer_update_callback();
  void discovered_peer_callback();

public:
  PeerConnectionManager(TorrentFile&, pdisconnection_queue&, pconnection_queue&);
  void run_manager();
  int get_listening_port();
};

#endif
