#include "Constants.hpp"
#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <cassert>
#include <cerrno>
#include <cstring>
#include <ev++.h>
#include <ev.h>
#include <netdb.h>
#include <netinet/in.h>
#include <ranges>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include "../Errorhandlers/BittorentErrors.hpp"
#include "PeerConnectionManager.hpp"
#include "PeerManagerTypes.hpp"
#include "ThreadMessageTypes.hpp"

void PeerConnectionManager::ipv6_default_server_sockstore() {
  std::memset(&server.store, 0, sizeof(sockaddr_in));
  server.store.ipv6.sin6_family = AF_INET6;
  server.store_len = sizeof(sockaddr_in6);
}

void PeerConnectionManager::ipv4_default_server_sockstore() {
  std::memset(&server.store, 0, sizeof(sockaddr_in6));
  server.store.ipv4.sin_family = AF_INET;
  server.store_len = sizeof(sockaddr_in);
}

void PeerConnectionManager::initialize_manager_watchers() {
  server_socket_watcher.set(event_loop);
  server_socket_watcher.set(server.socket, ev::READ);
  server_socket_watcher.set<PeerConnectionManager, &PeerConnectionManager::server_socket_callback>(this);
  discovered.consumer.set(event_loop);
  discovered.consumer.set<PeerConnectionManager, &PeerConnectionManager::drain_discovered>(this);
  disconnects.consumer.set(event_loop);
  disconnects.consumer.set<PeerConnectionManager, &PeerConnectionManager::drain_disconnected>(this);

}

void PeerConnectionManager::drain_discovered() {
  ipv4_peer_address addr;
  while (discovered.queue.pop(addr)) {
    ipv4_discovered_cache.push(std::move(addr));
  }
  if (establishing) return;
  establisher.send_notification();
}

void PeerConnectionManager::drain_disconnected() {
  establisher.send_notification();
}

void PeerConnectionManager::erase(PeerConnection& peer) {
  if (peer.IPv == pipv::ipv6)
    ipv6_peers.erase(peer.key.ipv6);
  else if (peer.IPv == pipv::ipv4 || peer.IPv == pipv::ipv4maskedv6)
    ipv4_peers.erase(peer.key.ipv4);
  else
   assert(false && "Peer with no ipv found in connection_manager erase function");
}

bool PeerConnectionManager::connect(PeerConnection& peer) {
  if (peer.source == psource::tcp_server) {
    erase(peer);
    return false;
  }
  if (peer.source == psource::tracker) {
    if (peer.generation==0 && peer.fail_stats.failures==0) {

    }
    auto& sockstruct = peer.store.ipv4_store;

    return true;
  }
  erase(peer);
  return false;
}

bool pmestablisher_t::discovered_peer_handler() {
  ipv4_peer_address addr;
  while ( manager.ipv4_discovered_cache.fresh_pop(addr)==true ) {
    auto [peer_, inserted] = manager.ipv4_peers.try_emplace(addr);
    if ( !inserted )
      continue;
    peer_key_t peer_key{ .ipv4=addr };
    auto& peer = peer_->second;
    manager.initialize_peer(peer, peer_key, pipv::ipv4, psource::tracker);
    peer.state = pstate::DISCOVERED;
    if (manager.connect(peer)) {
      peer.listener.for_sock.start();
      ++current_inflight;
      return true;
    } else continue;
  }
  return false;
}

bool pmestablisher_t::disconnected_peer_handler() {
  disconnect_update disconnected;
  while ( manager.disconnects.queue.pop(disconnected)==true ) {
    auto& peer = *const_cast<PeerConnection*>(disconnected.peer);
    if (peer.generation != disconnected.generation) {
    // This is incase a peer from the tcp_server disconnects and reconnects back quicker
    // Than this peer disconncted update was invoked meaning this disconnect update is stale
    // and most likely now the peer now is connected
      continue;
    }
    peer.state = pstate::DISCONNECTED;
    --manager.connected_peers_count;
    if (peer.tcp.perrno == PEER_SHUTDOWN || peer.source == psource::tcp_server) {
      manager.erase(peer);
      continue;
    }
    ++peer.fail_stats.failures;
    manager.acquire_peer(peer);
    if (manager.connect(peer)) {
      ++current_inflight;
      return true;
    } else continue;
  }
  return false;
}

bool pmestablisher_t::failed_peer_handler() {
  while ( manager.failed_peers.empty()==false ) {
    auto& peer = *manager.failed_peers.front();
    manager.failed_peers.pop();
    if (peer.fail_stats.failures >= bprotocol::constants::peer::max_reties) {
      manager.erase(peer);
      continue;
    }
    if (manager.connect(peer)) {
      ++current_inflight;
      return true;
    } else continue;
  }
  return false;
}

void pmestablisher_t::plus_mask_current(std::size_t spot) {
  current = (spot+1 == handlers_count) ? discovered : static_cast<spot_t>(spot+1);
}

void pmestablisher_t::round_robin_establisher_scheduler() {
  // This is a load balancer.
  for (; current_inflight < bprotocol::constants::max_inflight_conns; ) {
     std::size_t spot = static_cast<std::size_t>(current);
    if (current == discovered)
      empties[spot] = !discovered_peer_handler();
    else if (current == disconnected)
      empties[spot] = !disconnected_peer_handler();
    else if (current == failed)
      empties[spot] = !failed_peer_handler();
    if (empties[0] && empties[1] && empties[2])
      break;
    plus_mask_current(spot);
  }
}

pmestablisher_t::pmestablisher_t(PeerConnectionManager& __manager): manager(__manager) {
  daemon.set<pmestablisher_t, &pmestablisher_t::round_robin_establisher_scheduler>(this);
  daemon.set(manager.event_loop);
}

void pmestablisher_t::send_notification(){
  daemon.send();
}

void pmestablisher_t::single_resolve_notification(){
  --current_inflight;
  send_notification();
}

std::size_t pmestablisher_t::get_current_inflight() {
  return current_inflight;
}

void PeerConnectionManager::initialize_server_socket() {
  // create socket
  sockaddr* sock_addr = reinterpret_cast<sockaddr*>( &server.store );
  ipv6_default_server_sockstore();
  server.socket = socket(AF_INET6, server.flags, server.trspt_proto);
  if (server.socket<0)
    handle_socket_errno(errno);
  // put off ipv6 only
  if (sock_addr->sa_family == AF_INET6) {
    int ipv6only_off_return = setsockopt(server.socket, IPPROTO_IPV6, IPV6_V6ONLY, &server.off_ipv6only, sizeof(server.off_ipv6only));
    if (ipv6only_off_return == 0)
      server.ipv4_support = true;
    else
      handle_ip_errno(errno);
  }
  // bind socket
  if (sock_addr->sa_family == AF_INET6) {
    server.store.ipv6.sin6_addr = in6addr_any;
    server.store.ipv6.sin6_port = 0;
  } else {
    server.store.ipv4.sin_addr.s_addr = INADDR_ANY;
    server.store.ipv4.sin_port = 0;
  }
  while (true) {
    int bind_return = bind(server.socket, sock_addr , server.store_len);
    if (bind_return == 0) break;
    handle_bind_errno(errno);
  }
  // get listening port
  int get_sock_name_return = getsockname(server.socket, sock_addr , &server.store_len);
  if (get_sock_name_return != 0)
    throw Peer_Manager_SYS_Error{errno};
  server.port = ntohs( sock_addr->sa_family ==AF_INET6 ? server.store.ipv6.sin6_port : server.store.ipv4.sin_port);
  // mark as listening
  int listen_return = listen(server.socket, bprotocol::constants::connection_backlog);
  if (listen_return != 0)
    throw Peer_Manager_SYS_Error{errno};
}

int PeerConnectionManager::initialize_libev() {
  return ev::recommended_backends();
}

PeerConnectionManager::PeerConnectionManager(TorrentFile& a, pconnection_queue& b, pdisconnection_queue& c ,pdiscovery_queue_ipv4& d)
  :event_loop(initialize_libev()), torrent(a), connects(b), disconnects(c), discovered(d), establisher(*this) {
  ev_set_userdata(event_loop.raw_loop, this);
  initialize_server_socket();
  initialize_manager_watchers();
}


bool PeerConnectionManager::accept_peer_connection() {
  peer_sock_store_t new_store{};
  sockaddr* sock_addr = reinterpret_cast<sockaddr*>(&new_store);
  int accept_return = accept4(server.socket, (sockaddr*)&new_store, &server.store_len, SOCK_NONBLOCK);
  if (accept_return<0)
    return handle_server_errno(errno);

  // extract peer id
  pipv ip_version = pipv::null;
  peer_key_t peer_addr {};

  if (sock_addr->sa_family ==AF_INET6) {
    if (IN6_IS_ADDR_V4MAPPED(&new_store.ipv6_store.sin6_addr)) {
      ip_version = pipv::ipv4maskedv6;
      std::memcpy(&peer_addr.ipv4, &new_store.ipv6_store.sin6_addr.s6_addr[12], sizeof(in_addr) );
      std::memcpy(&peer_addr.ipv4.iport[4], &new_store.ipv4_store.sin_port, sizeof(in_port_t));
    } else {
      ip_version = pipv::ipv6;
      std::memcpy(&peer_addr.ipv6, &new_store.ipv6_store.sin6_addr, sizeof(in6_addr) );
      std::memcpy(&peer_addr.ipv6.iport[16], &new_store.ipv6_store.sin6_port, sizeof(in_port_t));
    }
  }
  else if (sock_addr->sa_family==AF_INET) {
    ip_version = pipv::ipv4;
    std::memcpy(&peer_addr.ipv4, &new_store.ipv4_store.sin_addr, sizeof(in_addr) );
    std::memcpy(&peer_addr.ipv4.iport[4], &new_store.ipv4_store.sin_port, sizeof(in_port_t));
  }
  else {
    assert(false && "Unexpected Address Family: accept_peer_connection");
  }

  PeerConnection* peer_view;
  bool inserted = false;
  if (ip_version == pipv::ipv4 || ip_version == pipv::ipv4maskedv6) {
    auto [it, __inserted] = ipv4_peers.try_emplace(peer_addr.ipv4);
    inserted = __inserted;
    peer_view = &it->second;
  } else if (ip_version == pipv::ipv6) {
    auto [it, __inserted] = ipv6_peers.try_emplace(peer_addr.ipv6);
    inserted = __inserted;
    peer_view = &it->second;
  } else
    assert(false && "failsafe, something wrong in accept_peer_connection");

  auto& peer = *peer_view;
  if (inserted) {
    initialize_peer(peer, peer_addr, ip_version, psource::tcp_server);
  } else {
    peer.generation++;
  }
  server_define_peer(peer, accept_return, &new_store);
  peer.state = pstate::HANDSHAKE;
  peer.listener.for_sock.start();
  return true;
}


void PeerConnectionManager::peer_socket_callback(ev::io& sw, int event) {
  PeerConnectionManager& manager = *static_cast<PeerConnectionManager*>(ev_userdata(sw.loop.raw_loop));
  PeerConnection& peer = *static_cast<PeerConnection*>(sw.data);
  if (event & ev::READ) {
    if (peer.state == pstate::DISCOVERED) {
    }

    if (peer.state == pstate::HANDSHAKE) {
      auto [recvd, pbuffer_full] = peer.recv();
      if (!recvd) return;
      if ( manager.parse_handshake(peer) == 1 ) {
        if (peer.source == psource::tcp_server) {
          manager.buffer_handshake(peer);
          auto [sent, pbuffer_empty] = peer.send();
          if (!sent) return;
        }
        peer.state = pstate::CONNECTED;
        manager.dispatch_connect(peer);
        manager.establisher.single_resolve_notification();
        return;
      }
      return;
    }

    if (peer.state == pstate::DISCONNECTED) {
    }
  }

  if (event & ev::WRITE) {
    if (peer.state == pstate::HANDSHAKE) {
      // handle partial handshake sends
      auto [sent, pbuffer_empty] = peer.send();
      if (!sent) return;
      return;
    }

    if (peer.state == pstate::DISCOVERED || peer.state == pstate::DISCONNECTED) {
      // This means peer was just discovered and the client of peermanager
      // just initiated a non block connect, so we should be expecting a
      // some update on the socket regarding connection establishment.
      int error;
      socklen_t err_var_len = sizeof error;
      int sock_opt_return = getsockopt(peer.tcp.socket, SOL_SOCKET, SO_ERROR, &error, &err_var_len);
      if (sock_opt_return<0)
        ; // DANGEROUS: handle socket option retrieval failure later
      else if (error != 0) {
        // peer.tcp.handle_errno(error);
        return;
      }
      // if function makes it here, peer has connected sucessfully.
      if (peer.state == pstate::DISCONNECTED) {
        peer.generation++;
      }
      peer.state = pstate::HANDSHAKE;
      manager.buffer_handshake(peer);
      auto [sent, pbuffer_empty] = peer.send();
      if (!sent) return;
    }

    if (peer.state == pstate::DISCONNECTED) {

    }
  }

  if (event & ev::ERROR) {

  }
}

void PeerConnectionManager::initialize_peer(PeerConnection& peer, peer_key_t& key, pipv ip_version, psource peer_source) {
  peer.listener.for_sock.set(event_loop);
  peer.listener.for_sock.set<&PeerConnectionManager::peer_socket_callback>();
  peer.listener.for_sock.data = &peer;

  peer.listener.for_timer.set(event_loop);
  peer.listener.for_timer.set<&PeerConnectionManager::peer_timer_callback>();
  peer.listener.for_timer.data = &peer;

  peer.id = get_id();
  std::memcpy(&peer.key, &key, sizeof(key));
  peer.IPv = ip_version;
  peer.source = peer_source;

  if (peer.source == psource::tracker) {
    if (peer.IPv == pipv::ipv4) {
      peer.store.ipv4_store.sin_family = AF_INET;
      std::memcpy(&peer.store.ipv4_store.sin_addr, &peer.key.ipv4.iport, 4);
      std::memcpy(&peer.store.ipv4_store.sin_port, &peer.key.ipv4.iport[5], 2);
    } else if (peer.IPv == pipv::ipv6 || peer.IPv == pipv::ipv4maskedv6) {
      peer.store.ipv6_store.sin6_family = AF_INET6;
      std::memcpy(&peer.store.ipv6_store.sin6_addr, &peer.key.ipv6.iport, 16);
      std::memcpy(&peer.store.ipv6_store.sin6_port, &peer.key.ipv6.iport[17], 2);
    }
  }
}

void PeerConnectionManager::server_define_peer(PeerConnection& peer, int socket, peer_sock_store_t* store) {
  peer.tcp.socket = socket;
  memcpy(&peer.store, store, server.store_len);
}

void PeerConnectionManager::release_peer(PeerConnection& peer) {
  peer.listener.for_sock.stop();
  peer.listener.for_timer.stop();
}

void PeerConnectionManager::dispatch_connect(PeerConnection& peer) {
  // send connected peer to transfer manager for management here.
  release_peer(peer);
  connected_peers_count++;
  connect_update new_connect { .peer=&peer, .socket=peer.tcp.socket, .id=peer.id, .generation=peer.generation };
  (void)connects.queue.push(std::move(new_connect));
  connects.consumer.send();
}

void PeerConnectionManager::server_socket_callback(ev::io& server, int event){
  (void)event;(void)server;
  bool pending_accepts = true;
  while (connected_peers_count<bprotocol::constants::healthy_peer_count && pending_accepts)
     pending_accepts = accept_peer_connection();
}

int PeerConnectionManager::get_listening_port() {
  return server.port;
}
