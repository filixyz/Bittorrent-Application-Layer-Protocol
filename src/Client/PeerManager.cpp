#include "PeerManager.h"
#include "Constants.h"
#include "DynamicBitset.h"
#include "TorrentFile.h"
#include <algorithm>
#include <arpa/inet.h>
#include <cassert>
#include <cerrno>
#include <cstring>
#include <ev++.h>
#include <ev.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>

std::pair<ssize_t, bool> PeerManager::PeerHandle::recv() {
  auto [io_vec, io_possible] = recv_buffer.prepare_write();
  ephemereal_hdr.msg_iov = io_vec.first;
  ephemereal_hdr.msg_iovlen = io_vec.second;
  auto recv_return = recvmsg(socket, &ephemereal_hdr, 0);
  return {recv_return, io_possible};
}

std::pair<ssize_t, bool> PeerManager::PeerHandle::send() {
  auto [io_vec, io_possible] = send_buffer.prepare_read();
  ephemereal_hdr.msg_iov = io_vec.first;
  ephemereal_hdr.msg_iovlen = io_vec.second;
  auto send_return = sendmsg(socket, &ephemereal_hdr, MSG_NOSIGNAL); // No sigpipe.
  return {send_return, io_possible};
}

PeerManager::PeerHandle::PeerHandle(std::string a, std::size_t b): peer_id(a), bitfield(b) {
  ephemereal_hdr.msg_name = nullptr;
  ephemereal_hdr.msg_control = nullptr;
}

PeerManager::PeerHandle PeerManager::PeerConnection::dummypeer{"dummy", 0};

void PeerManager::PeerConnection::set_endpoint(PeerHandle& peer_) {
  peer = &peer_;
}

bool PeerManager::PeerConnection::is_dummy() {
  return peer == &dummypeer;
}

void PeerManager::PeerConnection::endpoint_disconnected() {
  peer            = &dummypeer;
  down_rate       = upld_rate           = 0;
  choked_by_them  = choked_by_them      = true;
  them_interested = interested_in_them  = false;
}

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

PeerManager::PeerManager(TorrentFile& torrent_p)
  : event_loop(initialize_libev()), torrent(torrent_p), peer_connections(bprotocol::constants::healthy_peer_count*2) {
  ev_set_userdata(event_loop.raw_loop, this);
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
  int accept_return = accept4(server.socket, (sockaddr*)&new_store, &server.store_len, SOCK_NONBLOCK);
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
  if (!inserted)
    ; // This is a reconnect
  auto& new_or_found_peer = peer_ref->second;
  new_or_found_peer.socket = accept_return;
  new_or_found_peer.state=PeerHandle::S_HANDSHAKE;
  memcpy(&new_or_found_peer.store, &new_store, server.store_len);
  acquire_peer(new_or_found_peer);

  return true;
}

void PeerManager::acquire_peer(PeerHandle& peer) {
  peer.socket_watcher.set(peer.socket, ev::READ);
  peer.socket_watcher.set<&PeerManager::peer_socket_callback>();
  peer.socket_watcher.data = &peer;
  peer.socket_watcher.set(event_loop);
  peer.socket_watcher.start();
}

void PeerManager::peer_socket_callback(ev::io& sw, int event) {
  PeerManager& manager = *static_cast<PeerManager*>(ev_userdata(sw.loop.raw_loop));
  PeerHandle& peer = *static_cast<PeerHandle*>(sw.data);
  if (event & ev::READ) {
    if (peer.state == PeerHandle::C_HANDSHAKE || peer.state == PeerHandle::S_HANDSHAKE) {
      auto [recv_return, pbuffer_full] = peer.recv();
      if (recv_return<0) {
        handle_peer_errno(errno, peer);
        return;
      }
      peer.recv_buffer.commit_write(recv_return);
      if ( manager.parse_handshake(peer) ) {
        if (peer.state == PeerHandle::S_HANDSHAKE)
          manager.send_handshake(peer);
        peer.state = PeerHandle::CONNECTED;
        manager.add_connected_peer(peer);
        return;
      }
      return;
    }
  }

  if (event & ev::WRITE) {
    // handle partial handshake sends
    if (peer.state == PeerHandle::C_HANDSHAKE || peer.state == PeerHandle::S_HANDSHAKE) {
      auto [send_return, pbuffer_empty] = peer.send();
      if (send_return<0) {
        handle_peer_errno(errno, peer);
        return;
      }
      peer.send_buffer.commit_read(send_return);
      return;
    }
  }

  if (event & ev::ERROR) {

  }
}

void PeerManager::add_connected_peer(PeerHandle& peer) {
  for (auto& connection : peer_connections) {
    if (connection.is_dummy()) {
      connection.set_endpoint(peer);
      ++connected_peers_count;
      return;
    }
  }
  // send connected peer to transfer manager for management here.
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
