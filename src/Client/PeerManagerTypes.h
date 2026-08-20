#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <mutex>
#include <queue>
#include <sys/types.h>
#include <unistd.h>
#include <utility>
#include <vector>
#include <string>
#include <bitset>
#include <ev++.h>
#include <memory>
#include "DynamicBitset.h"
#include "TCPRingBuffer.h"
#include "Randomer.h"
#include "../Errorhandlers/BittorentErrors.h"
#include "ThreadMessageTypes.h"
#define XXH_INLINE_ALL
#include "xxhash.h"
//Unix Networking Headers here
#include <sys/socket.h>
#include <fcntl.h>
#include <signal.h>
#include <netinet/in.h>
#include <cerrno>
#include <arpa/inet.h>
#include <netdb.h>

inline constexpr int PEER_SHUTDOWN = -100;

class PeerTransferManager;
class PeerConnectionManager;
struct PeerConnection;
struct PeerSession;

struct transact {
  bool success;   // if true peer is still connected, otherwise false
  bool bufferred; // if true IO happened with application layer buffer, otherwise false.
};

using peer_id_t = std::array<std::byte, 20>;

struct tcp_server_context
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

struct peer_nonblock_tcp {
  int socket;
  int perrno;
  msghdr ephemereal_hdr{};
  transact send(hanshake_buffer&);
  transact recv(hanshake_buffer&);
  transact recv(session_buffer&);
  transact send(session_buffer&);
private:
  bool handle_errno(int);
  bool connect();
  bool close();
  bool disconnect();
  ssize_t recv(prepare_t, transact&);
  ssize_t send(prepare_t, transact&);
  friend PeerConnectionManager;
};

struct peer_watchers {
  ev::io for_sock;
  ev::timer for_timer;
};

enum class pstate: std::uint8_t {DISCOVERED, S_HANDSHAKE, C_HANDSHAKE, CONNECTED, DISCONNECTED, DEAD};
enum class psource: std::uint8_t {null, tracker, tcp_server};

struct PeerConnection {
  peer_nonblock_tcp tcp;
  union { sockaddr_in ipv4_store; sockaddr_in6 ipv6_store; } store{};
  hanshake_buffer recv_buffer{};
  hanshake_buffer send_buffer{};
  pstate state {pstate::DISCOVERED};
  psource source {psource::null};
  peer_id_t peer_id;
  std::size_t id;
  std::size_t generation{0};
  peer_watchers listener;
  transact recv();
  transact send();
};

struct PeerSession {
  peer_nonblock_tcp tcp;
  session_buffer recv_buffer{};
  session_buffer send_buffer{};
  std::size_t id;
  std::size_t generation;
  enum from { me=0, them=1 };
  std::bitset<2> choke {};
  std::bitset<2> interest {};
  std::size_t down_rate{0};
  std::size_t upld_rate{0};
  peer_watchers watcher;
  DynamicBitset bitfield;
  void set_endpoint(const PeerConnection*);
  bool is_dummy();
  const PeerConnection* endpoint_disconnected();
  transact send();
  transact recv();
private:
  const PeerConnection* peer {&dummypeer};
  static PeerConnection dummypeer;
};

struct connect_update {
  PeerConnection* peer;
  int socket;
  std::size_t id;
  std::size_t generation;
};

struct disconnect_update {
  PeerConnection* peer;
  int perrno;
};

template<class T>
struct consumer_queue {
  std::queue<T> queue;
  ev::async consumer;
};

using pconnection_queue = consumer_queue<connect_update>;
using pdisconnection_queue = consumer_queue<disconnect_update>;
using pdiscovery_queue_ipv4 = consumer_queue<ipv4_peer_address>;

struct peer_manager_hashers {
  std::size_t operator()(const ipv4_peer_address& key) const noexcept {
    return XXH3_64bits(key.iport.data(), key.iport.size());
  }
  std::size_t operator()(const ipv6_peer_address& key) const noexcept {
    return XXH3_64bits(key.iport.data(), key.iport.size());
  }
  std::size_t operator()(const peer_id_t& key) const noexcept {
    return XXH3_64bits(key.data(), key.size());
  }
};

struct peer_id_gen {
  std::size_t id{0};
  std::size_t operator()() { return id++; }
};

// declaration migh tbe errornewous
// might need its own .cpp
static peer_id_gen new_peer_id;
