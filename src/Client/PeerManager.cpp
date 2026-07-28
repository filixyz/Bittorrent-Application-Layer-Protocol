#include "PeerManager.h"
#include "Randomer.h"
#include <cerrno>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>

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
      throw NO_IP_Support{};
  }
  if (error == ENOMEM)
    throw NO_Memory{};
}

void PeerManager::handle_ip_errno(int) {

}

void PeerManager::handle_bind_errno(int error) {
  if (error == EADDRINUSE) {
    ;
  }
}

void PeerManager::initialize_server_socket() {
  // create socket
  ipv6_default_server_sockstore();
  server.socket = socket(server.store.ss_family, server.flags, server.trspt_proto);
  if (server.socket<0) handle_socket_errno(errno);
  int ipv6only_off_return = setsockopt(server.socket, IPPROTO_IPV6, IPV6_V6ONLY, &server.off_ipv6only, sizeof(int));
  if (ipv6only_off_return != 0) handle_ip_errno(errno);

  // bind socket
  if (server.store.ss_family == AF_INET6) {
    sockaddr_in6* ipv6_intf = (sockaddr_in6*) &server.store;
    ipv6_intf->sin6_addr = in6addr_any;
    ipv6_intf->sin6_port = htons(server.new_port.get());
  } else {
    sockaddr_in* ipv4_intf = (sockaddr_in*) &server.store;
    ipv4_intf->sin_addr.s_addr = INADDR_ANY;
    ipv4_intf->sin_port = htons(server.new_port.get());
  }
  do {
    int bind_return = bind(server.socket, (sockaddr*)(&server.store), sizeof(server.store_len));
    if (bind_return == 0) break;
    handle_bind_errno(errno);
  } while( true );

  // mark as listening
  int listen_return = listen(server.socket, 0);
  if(listen_return != 0)
    ;

  // set callback for io watcher
}
