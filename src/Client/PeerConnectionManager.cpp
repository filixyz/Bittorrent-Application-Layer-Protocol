#include "Constants.h"
#include <arpa/inet.h>
#include <asm-generic/socket.h>
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
#include "PeerConnectionManager.h"
#include "PeerManagerTypes.h"


void PeerConnectionManager::ipv6_default_server_sockstore() {
  std::memset(&server.store, 0, sizeof(sockaddr_storage));
  server.store.ss_family = AF_INET6;
  server.store_len = sizeof(sockaddr_in6);
}

void PeerConnectionManager::ipv4_default_server_sockstore() {
  std::memset(&server.store, 0, sizeof(sockaddr_storage));
  server.store.ss_family = AF_INET;
  server.store_len = sizeof(sockaddr_in);
}

void PeerConnectionManager::initialize_manager_watchers() {
  // init server socket watcher
  server_socket_watcher.set(event_loop);
  server_socket_watcher.set(server.socket, ev::READ);
  server_socket_watcher.set<PeerConnectionManager, &PeerConnectionManager::server_socket_callback>(this);
  // init queue consumer watcher
  queue_consumer_watcher.set(event_loop);
  queue_consumer_watcher.set<PeerConnectionManager, &PeerConnectionManager::discovered_peer_callback>(this);
  // init peer_update_watcher
  peer_update_watcher.set(event_loop);
  peer_update_watcher.set<PeerConnectionManager, &PeerConnectionManager::peer_update_callback>(this);
}


void PeerConnectionManager::initialize_server_socket() {
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

int PeerConnectionManager::initialize_libev() {
  return ev::recommended_backends();
}

PeerConnectionManager::PeerConnectionManager(TorrentFile& torrent_p, pdisconnection_queue& d, pconnection_queue& c)
  :event_loop(initialize_libev()), torrent(torrent_p), connects(c), disconnects(d) {
  ev_set_userdata(event_loop.raw_loop, this);
  initialize_server_socket();
  initialize_manager_watchers();
}


bool PeerConnectionManager::accept_peer_connection() {
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
  if (!inserted) {
    // This is a reconnect
    peer_ref->second.generation++;
  }
  auto& new_or_found_peer = peer_ref->second;
  new_or_found_peer.socket = accept_return;
  new_or_found_peer.state=PeerConnection::S_HANDSHAKE;
  new_or_found_peer.id = peer_handles.size();
  memcpy(&new_or_found_peer.store, &new_store, server.store_len);
  acquire_peer(new_or_found_peer);

  return true;
}

void PeerConnectionManager::acquire_peer(PeerConnection& peer) {
  peer.socket_watcher.set(peer.socket, ev::READ);
  peer.socket_watcher.set<&PeerConnectionManager::peer_socket_callback>();
  peer.socket_watcher.data = &peer;
  peer.socket_watcher.set(event_loop);
  peer.socket_watcher.start();
}

void PeerConnectionManager::peer_socket_callback(ev::io& sw, int event) {
  PeerConnectionManager& manager = *static_cast<PeerConnectionManager*>(ev_userdata(sw.loop.raw_loop));
  PeerConnection& peer = *static_cast<PeerConnection*>(sw.data);
  if (event & ev::READ) {
    if (peer.state == PeerConnection::DISCOVERED) {
    }

    if (peer.state == PeerConnection::C_HANDSHAKE || peer.state == PeerConnection::S_HANDSHAKE) {
      auto [recvd, pbuffer_full] = peer.recv();
      if (!recvd) return;
      if ( manager.parse_handshake(peer) == 1 ) {
        if (peer.state == PeerConnection::S_HANDSHAKE) {
          manager.buffer_handshake(peer);
          auto [sent, pbuffer_empty] = peer.send();
          if (!sent) return;
        }
        peer.state = PeerConnection::CONNECTED;
        manager.add_connected_peer(peer);
        return;
      }
      return;
    }
  }

  if (event & ev::WRITE) {
    if (peer.state == PeerConnection::C_HANDSHAKE || peer.state == PeerConnection::S_HANDSHAKE) {
      // handle partial handshake sends
      auto [sent, pbuffer_empty] = peer.send();
      if (!sent) return;
      return;
    }

    if (peer.state == PeerConnection::DISCOVERED) {
      // This means peer was just discovered and the client of peermanager
      // just initiated a non block connect, so we should be expecting a
      // some update on the socket regarding connection establishment.
      int error;
      socklen_t err_var_len = sizeof error;
      int sock_opt_return = getsockopt(peer.socket, SOL_SOCKET, SO_ERROR, &error, &err_var_len);
      if (sock_opt_return<0)
        ; // DANGEROUS: handle socket option retrieval failure later
      else if (error != 0) {
        peer.handle_errno(error);
        return;
      }
      // if function makes it here, peer has connected sucessfully.
      manager.buffer_handshake(peer);
      auto [sent, pbuffer_empty] = peer.send();
      if (!sent) return;
      peer.state = PeerConnection::C_HANDSHAKE;
    }
  }

  if (event & ev::ERROR) {

  }
}

void PeerConnectionManager::add_connected_peer(PeerConnection& peer) {
  // send connected peer to transfer manager for management here.
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
