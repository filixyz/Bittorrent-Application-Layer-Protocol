#ifndef PEER_MANAGER
#define PEER_MANAGER

#include <atomic>
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
    enum pstate: std::uint8_t {DISCOVERED, S_HANDSHAKE, C_HANDSHAKE, CONNECTED, DISCONNECTED, DEAD};
    pstate state {DISCOVERED};
    const std::string peer_id{""};
    int socket{};
    sockaddr_storage store{};
    DynamicBitset bitfield;
    tcp_buffer recv_buffer{};
    tcp_buffer send_buffer{};
    ev::io socket_watcher;
    ev::timer timer_watcher;
    msghdr ephemereal_hdr{};
    std::pair<bool, bool> recv();
    std::pair<bool, bool> send();
    PeerHandle(std::string, std::size_t);
  private:
    void connect();
    void disconnect();
    // returns: false if error is irrecovereable (disconnected/disconnection candidate)
    // true if error recovereable eg EAGAIN/EWOULDBLOCK/EINTR (connected with issues handled/expected)
    bool handle_errno(int);
    friend PeerManager;
  };
  struct PeerConnection
  {
    // non atomic members are only to be touched
    // by currently owning thread.
    PeerHandle* peer {&dummypeer};
    // active is only set false by TransferManager before handoff
    // and it is only set true by PeerManager before handoff
    bool active{false};
    std::atomic<bool> choking_them{true};
    std::atomic<bool> choked_by_them{true};
    std::atomic<bool> interested_in_them{false};
    std::atomic<bool> them_interested{false};
    std::atomic<std::size_t> down_rate{0};
    std::atomic<std::size_t> upld_rate{0};
  private:
    static PeerHandle dummypeer;
    void set_endpoint(PeerHandle&);
    bool is_dummy();
    void endpoint_disconnected();
    friend PeerManager;
  };

  // returns (while acting accordingly)
  // -1 if encountered malformed handshake
  //  0 if incomplete handshake message or no handshake
  //  1 if sucessful
  int parse_handshake(PeerHandle&);
  // return number of bytes buffered for send
  int buffer_handshake(PeerHandle&);

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
  std::vector<PeerConnection> peer_connections{};
  // std::mutex apl_mutex; // for active_peerlist
  // std::vector<PeerConnection*> active_peers_list{};

  void ipv6_default_server_sockstore();
  void ipv4_default_server_sockstore();

  void handle_socket_errno(int);
  void handle_ip_errno(int);
  void handle_bind_errno(int);

  int  initialize_libev();
  void initialize_server_socket();
  void initialize_manager_watchers();
  void initialize_peer_pool();

  void acquire_peer(PeerHandle&);
  void release_peer(PeerHandle&);

  void add_connected_peer(PeerHandle&);
  bool handle_server_errno(int);
  bool accept_peer_connection();
  void server_socket_callback(ev::io&, int);

  void static handle_peer_errno(int, PeerHandle&);
  void static peer_socket_callback(ev::io&, int);

  void optimistic_unchoke();
  void rankify_peers_callback();
  void peer_update_callback();
  void discovered_peer_callback();

public:
  PeerManager(TorrentFile&);
  void run_manager();
  int get_listening_port();
  // friend TransferManager;
};

#endif
