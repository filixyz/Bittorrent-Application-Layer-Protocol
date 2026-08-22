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
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include "PeerConnectionManager.hpp"
#include "PeerManagerTypes.hpp"

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
  server_socket_watcher.set(event_loop);
  server_socket_watcher.set(server.socket, ev::READ);
  server_socket_watcher.set<PeerConnectionManager, &PeerConnectionManager::server_socket_callback>(this);
  discovered.consumer.set(event_loop);
  discovered.consumer.set<PeerConnectionManager, &PeerConnectionManager::establish_connections>(this);
  disconnects.consumer.set(event_loop);
  disconnects.consumer.set<PeerConnectionManager, &PeerConnectionManager::reastablish_connections>(this);
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

PeerConnectionManager::PeerConnectionManager(TorrentFile& a, pconnection_queue& b, pdisconnection_queue& c ,pdiscovery_queue_ipv4& d)
  :event_loop(initialize_libev()), torrent(a), connects(b), disconnects(c), discovered(d) {
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
  auto [peer_ref, inserted] = ipv4_peers.try_emplace({});
  if (!inserted) {
   // peer_ref->second.gene ration++;
  }
  auto& peer = peer_ref->second;
  peer.tcp.socket = accept_return;
  peer.state = pstate::S_HANDSHAKE;
  peer.source = psource::tcp_server;
  peer.id = get_id();
  memcpy(&peer.store, &new_store, server.store_len);
  acquire_peer(peer);
  return true;
}

void PeerConnectionManager::initiate_new_connection() {
  if (discovered.queue.empty() || connected_peers_count>=bprotocol::constants::healthy_peer_count)
    return;
  auto peer = std::move(discovered.queue.front());
  discovered.queue.pop();
//  if (peer_handles.find(peer))

}

void PeerConnectionManager::peer_socket_callback(ev::io& sw, int event) {
  PeerConnectionManager& manager = *static_cast<PeerConnectionManager*>(ev_userdata(sw.loop.raw_loop));
  PeerConnection& peer = *static_cast<PeerConnection*>(sw.data);
  if (event & ev::READ) {
    if (peer.state == pstate::DISCOVERED) {
    }

    if (peer.state == pstate::C_HANDSHAKE || peer.state == pstate::S_HANDSHAKE) {
      auto [recvd, pbuffer_full] = peer.recv();
      if (!recvd) return;
      if ( manager.parse_handshake(peer) == 1 ) {
        if (peer.state == pstate::S_HANDSHAKE) {
          manager.buffer_handshake(peer);
          auto [sent, pbuffer_empty] = peer.send();
          if (!sent) return;
        }
        peer.state = pstate::CONNECTED;
        manager.dispatch_connect(peer);
        manager.initiate_new_connection();
        return;
      }
      return;
    }

    if (peer.state == pstate::DISCONNECTED) {
    }
  }

  if (event & ev::WRITE) {
    if (peer.state == pstate::C_HANDSHAKE || peer.state == pstate::S_HANDSHAKE) {
      // handle partial handshake sends
      auto [sent, pbuffer_empty] = peer.send();
      if (!sent) return;
      return;
    }

    if (peer.state == pstate::DISCOVERED) {
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
      manager.buffer_handshake(peer);
      auto [sent, pbuffer_empty] = peer.send();
      if (!sent) return;
      peer.state = pstate::C_HANDSHAKE;
    }

    if (peer.state == pstate::DISCONNECTED) {

    }
  }

  if (event & ev::ERROR) {

  }
}

void PeerConnectionManager::handle_disconnect(PeerConnection& peer) {
  // acquire_peer(peer);
  // add to recconnection container
  --connected_peers_count;
}

void PeerConnectionManager::acquire_peer(PeerConnection& peer) {
  peer.listener.for_sock.set(peer.tcp.socket, ev::READ);
  peer.listener.for_sock.set<&PeerConnectionManager::peer_socket_callback>();
  peer.listener.for_sock.data = &peer;
  peer.listener.for_sock.set(event_loop);
  peer.listener.for_sock.start();
}

void PeerConnectionManager::release_peer(PeerConnection& peer) {
  peer.listener.for_sock.stop();
  peer.listener.for_timer.stop();
}

void PeerConnectionManager::dispatch_connect(PeerConnection& peer) {
  // send connected peer to transfer manager for management here.
  release_peer(peer);
  ++peer.generation;
  ++connected_peers_count;
  connect_update new_connect { .peer=&peer, .id=peer.id, .generation=peer.generation };
  connects.queue.push(std::move(new_connect));
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
