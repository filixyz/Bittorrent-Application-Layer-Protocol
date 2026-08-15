#include "PeerConnectionManager.h"
#include <cstring>


bool PeerConnectionManager::handle_server_errno(int error){
  if (error == EAGAIN) return false;
  if (error == EWOULDBLOCK) return false;
  if (error == EINTR) return true;
  if (error == ECONNABORTED) return true;
  return false;
}

void PeerConnectionManager::handle_socket_errno(int error) {
  if (error == EAFNOSUPPORT) {
    ipv4_default_server_sockstore();
    server.socket = socket(server.store.ss_family, server.flags, server.trspt_proto);
    if (server.socket<0)
      throw Peer_Manager_SYS_Error{error};
    server.ipv4_support=true;
    return;
  }
  if (error == ENOMEM) {
    throw Peer_Manager_SYS_Error{error};
  }
  throw Peer_Manager_SYS_Error{error};
}

void PeerConnectionManager::handle_ip_errno(int error) {
  if (error == ENODEV) {
    server.ipv4_support = false;
    return;
  }
  throw Peer_Manager_SYS_Error{error};
}

void PeerConnectionManager::handle_bind_errno(int error) {
  if (error == EADDRINUSE) {}
  throw Peer_Manager_SYS_Error{error};
}
