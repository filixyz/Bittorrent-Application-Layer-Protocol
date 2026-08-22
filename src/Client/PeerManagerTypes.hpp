#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <queue>
#include <sys/types.h>
#include <unistd.h>
#include <utility>
#include <bitset>
#include <ev++.h>
#include "Constants.hpp"
#include "DynamicBitset.hpp"
#include "TCPRingBuffer.hpp"
#include "ThreadMessageTypes.hpp"
#define XXH_INLINE_ALL
#include "xxhash.h"
//Unix Networking Headers here
#include <sys/socket.h>
#include <fcntl.h>
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
  int socket{-1};
  int perrno{-1};
  msghdr ephemereal_hdr{};

  template<std::size_t N>
  transact send(tcp_buffer<N>& buffer) {
    prepare_t prepare = buffer.prepare_read();
    ephemereal_hdr.msg_iov = prepare.iovec.first;
    ephemereal_hdr.msg_iovlen = prepare.iovec.second;

    ssize_t send_return; do {
      send_return = sendmsg(socket, &ephemereal_hdr, MSG_NOSIGNAL);
    } while (send_return<0 && errno == EINTR);

    if (send_return<0)
      return {handle_send_perrno(errno), prepare.buffered};
    perrno = -1;
    buffer.commit_read(send_return);
    return {true, prepare.buffered};
  }

  template<std::size_t N>
  transact recv(tcp_buffer<N>& buffer) {
    prepare_t prepare = buffer.prepare_write();
    ephemereal_hdr.msg_iov = prepare.iovec.first;
    ephemereal_hdr.msg_iovlen = prepare.iovec.second;

    ssize_t recv_return; do {
      recv_return = recvmsg(socket, &ephemereal_hdr, 0);
    } while (recv_return<0 && errno == EINTR);

    if (recv_return == 0) {
      perrno = PEER_SHUTDOWN;
      return {false, prepare.buffered};
    } else if (recv_return<0) {
      return {handle_recv_perrno(errno), prepare.buffered};
    }
    perrno = -1;
    buffer.commit_write(recv_return);
    return {true, prepare.buffered};
  }

private:
  bool handle_send_perrno(int);
  bool handle_recv_perrno(int);
  bool handle_connect_perrno(int);
  bool pconnect(const sockaddr*);
  void pclose();
  void disconnect();
  friend PeerConnectionManager;
};

struct peer_watchers {
  ev::io for_sock;
  ev::timer for_timer;
};

enum class pstate: std::uint8_t {DISCOVERED, S_HANDSHAKE, C_HANDSHAKE, CONNECTED, DISCONNECTED, DEAD};
enum class psource: std::uint8_t {null, tracker, tcp_server};
using peer_id_t = std::array<std::byte, 20>;

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
  const PeerConnection* peer;
  int socket;
  std::size_t id;
  std::size_t generation;
};

struct disconnect_update {
  const PeerConnection* peer;
  int perrno;
};

#include "spsc_queue.hpp"

template<class T>
struct consumer_queue {
  spsc_queue<T, bprotocol::constants::healthy_peer_count> queue;
  ev::async notification;
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
