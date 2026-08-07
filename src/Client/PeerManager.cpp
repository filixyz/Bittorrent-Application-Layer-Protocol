#include "PeerManager.h"
#include "Constants.h"
#include "TorrentFile.h"
#include <arpa/inet.h>
#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <memory>
#include <netdb.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <tuple>
#include <utility>

PeerManager::PeerHandle PeerManager::PeerConnection::nullpeer{"nullpeer", 0};

void PeerManager::ipv6_default_server_sockstore() {
  std::memset(&server.store, 0, sizeof(sockaddr_storage));
  server.store.ss_family = AF_INET6;
  server.store_len = sizeof(sockaddr_in6);
}

void PeerManager::ipv4_default_server_sockstore() {
  std::memset(&server.store, 0, sizeof(sockaddr_storage));
  server.store.ss_family = AF_INET;
  server.store_len = sizeof(sockaddr_in);
}

void PeerManager::handle_socket_errno(int error) {
  if (error == EAFNOSUPPORT) {
    ipv4_default_server_sockstore();
    server.socket = socket(server.store.ss_family, server.flags, server.trspt_proto);
    if (server.socket<0 && errno==EAFNOSUPPORT)
      throw Peer_Manager_SYS_Error{error};
    else
      server.ipv4_support=true;
    return;
  }
  if (error == ENOMEM) {
    throw Peer_Manager_SYS_Error{error};
  }
  throw Peer_Manager_SYS_Error{error};
}

void PeerManager::handle_ip_errno(int error) {
  if (error == ENODEV) {
    server.ipv4_support = false;
    return;
  }
  throw Peer_Manager_SYS_Error{error};
}

void PeerManager::handle_bind_errno(int error) {
  if (error == EADDRINUSE) {}
  throw Peer_Manager_SYS_Error{error};
}

int PeerManager::initialize_libev() {
  return ev::recommended_backends();
}

void PeerManager::initialize_manager_watchers() {
  // init server socket watcher
  server_socket_watcher.set(event_loop);
  server_socket_watcher.set(server.socket, ev::READ);
  server_socket_watcher.set<PeerManager, &PeerManager::server_socket_callback>(this);
  // init peer_pool_timer
  peer_pool_timer.set(event_loop);
  peer_pool_timer.set(bprotocol::constants::rankify_duration);
  peer_pool_timer.set<PeerManager, &PeerManager::rankify_peers_callback>(this);
  // init queue consumer watcher
  queue_consumer_watcher.set(event_loop);
  queue_consumer_watcher.set<PeerManager, &PeerManager::discovered_peer_callback>(this);
  // init peer_update_watcher
  peer_update_watcher.set(event_loop);
  peer_update_watcher.set<PeerManager, &PeerManager::peer_update_callback>(this);
}


void PeerManager::initialize_server_socket() {
  // create socket
  ipv6_default_server_sockstore();
  server.socket = socket(server.store.ss_family, server.flags, server.trspt_proto);
  if (server.socket<0)
    handle_socket_errno(errno);
  // put off ipv6 only
  if (server.store.ss_family == AF_INET6) {
    int ipv6only_off_return = setsockopt(server.socket, IPPROTO_IPV6, IPV6_V6ONLY, &server.off_ipv6only, sizeof(server.off_ipv6only));
    if (ipv6only_off_return == 0)
      server.ipv4_support = true;
    else
      handle_ip_errno(errno);
  }
  // bind socket
  if (server.store.ss_family == AF_INET6) {
    sockaddr_in6* ipv6_intf = (sockaddr_in6*) &server.store;
    ipv6_intf->sin6_addr = in6addr_any;
    ipv6_intf->sin6_port = 0;
  } else {
    sockaddr_in* ipv4_intf = (sockaddr_in*) &server.store;
    ipv4_intf->sin_addr.s_addr = INADDR_ANY;
    ipv4_intf->sin_port = 0;
  }
  while (true) {
    int bind_return = bind(server.socket, (sockaddr*)(&server.store), server.store_len);
    if (bind_return == 0) break;
    handle_bind_errno(errno);
  }
  // get listening port
  int get_sock_name_return = getsockname(server.socket, (sockaddr*) &server.store , &server.store_len);
  if (get_sock_name_return != 0)
    throw Peer_Manager_SYS_Error{errno};
  server.port = ntohs( server.store.ss_family==AF_INET6 ?
    ((sockaddr_in6*)&server.store)->sin6_port : ((sockaddr_in*)&server.store)->sin_port
  );
  // mark as listening
  int listen_return = listen(server.socket, bprotocol::constants::connection_backlog);
  if (listen_return != 0)
    throw Peer_Manager_SYS_Error{errno};
}

PeerManager::PeerManager(TorrentFile& torrent_p): event_loop(initialize_libev()), torrent(torrent_p) {
  initialize_server_socket();
  initialize_manager_watchers();
}

bool PeerManager::handle_server_errno(int error){
  if (error == EAGAIN) return false;
  if (error == EWOULDBLOCK) return false;
  if (error == EINTR) return true;
  if (error == ECONNABORTED) return true;
  return false;
}

bool PeerManager::accept_peer_connection() {
  sockaddr_storage new_store{};
  int accept_return = accept(server.socket, (sockaddr*)&new_store, &server.store_len);
  if (accept_return<0)
    return handle_server_errno(errno);

  // extract peer id
  sa_family_t peer_family;
  in_port_t   peer_port;
  void*       peer_addr_src;
  if (server.store.ss_family==AF_INET6) {
    sockaddr_in6* peer6 = (sockaddr_in6*) &new_store;
    if (IN6_IS_ADDR_V4MAPPED(&peer6->sin6_addr)) {
      peer_family = AF_INET;
      peer_addr_src = &peer6->sin6_addr.s6_addr[12];
    } else {
      peer_family = AF_INET6;
      peer_addr_src = &peer6->sin6_addr;
    }
    peer_port = ntohs(peer6->sin6_port);
  }
  else if (server.store.ss_family==AF_INET) {
    sockaddr_in* peer4 = (sockaddr_in*) &new_store;
    peer_family = AF_INET;
    peer_addr_src = &peer4->sin_addr;
    peer_port = ntohs(peer4->sin_port);
  }
  else {
    assert(false && "Unexpected Address Family: accept_peer_connection");
  }
  char peeripvsbuf [INET6_ADDRSTRLEN];
  inet_ntop(peer_family, peer_addr_src, peeripvsbuf, sizeof peeripvsbuf);
  std::string peer_id = std::string{peeripvsbuf} + ':' + std::to_string(peer_port);

  // tries to emplace new peer or updates peer if already disocvered before
  auto [peer_ref, inserted] = peer_handles.try_emplace(peer_id, peer_id, torrent.get_piece_length());
  if (!inserted) {
    ; // This is a reconnect
  }
  auto& new_or_found_peer = peer_ref->second;
  new_or_found_peer.socket = accept_return;
  new_or_found_peer.state=PeerHandle::CONNECTED;
  memcpy(&new_or_found_peer.store, &new_store, server.store_len);
  add_connected_peer(new_or_found_peer);
  // TransferManager.acquire_watchers(new_peer);
  return true;
}

void PeerManager::add_connected_peer(PeerHandle& peer) {
  PeerConnection new_conn{};
  new_conn.peer = &peer;
  peer_pool_mutex.lock();
  peer_connections.push_back(std::move(new_conn));
  connected_peers_count++;
  peer_pool_mutex.unlock();
}

void PeerManager::server_socket_callback(ev::io& server, int event){
  (void)event;(void)server;
  bool pending_accepts = true;
  while (connected_peers_count<bprotocol::constants::healthy_peer_count && pending_accepts)
     pending_accepts = accept_peer_connection();
}

int PeerManager::get_listening_port() {
  return server.port;
}
