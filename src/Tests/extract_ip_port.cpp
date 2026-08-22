#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

std::string extract_ip_port_1(sockaddr_storage& store) {
  int addr_family;
  void* addr_source;
  int port;
  if (store.ss_family==AF_INET6) {
    sockaddr_in6* peer6 = (sockaddr_in6*) &store;
    if(IN6_IS_ADDR_V4MAPPED(&peer6->sin6_addr)) {
      addr_family = AF_INET;
      addr_source = &peer6->sin6_addr.s6_addr[12];
    } else {
      addr_family = AF_INET6;
      addr_source = &peer6->sin6_addr;
    }
    port = ntohs(peer6->sin6_port);
  }
  if (store.ss_family==AF_INET) {
    sockaddr_in* peer4 = (sockaddr_in*) &store;
    addr_family = AF_INET;
    addr_source = &peer4->sin_addr;
    port = ntohs(peer4->sin_port);
  }
  char ipvsbuf [INET6_ADDRSTRLEN];
  inet_ntop(addr_family, addr_source, ipvsbuf, sizeof ipvsbuf);
  return std::string{ipvsbuf} + ':' + std::to_string(port);
}

std::string extract_ip_port_2(sockaddr_storage& store) {
  std::string peer_id{};
  // extract peer id
  if (store.ss_family==AF_INET6) {
    sockaddr_in6* peer6 = (sockaddr_in6*) &store;
    if (IN6_IS_ADDR_V4MAPPED(&peer6->sin6_addr)) {
      uint32_t peer6ipv4;
      memcpy(&peer6ipv4, &peer6->sin6_addr.s6_addr[12], sizeof peer6ipv4);
      char ipv4sbuf [INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &peer6ipv4, ipv4sbuf, INET_ADDRSTRLEN);
      peer_id = std::string(ipv4sbuf) + ':' + std::to_string(peer6->sin6_port);
    }
    else {
      char ipv6sbuf [INET6_ADDRSTRLEN];
      inet_ntop(AF_INET6, &peer6->sin6_addr, ipv6sbuf, INET6_ADDRSTRLEN);
      peer_id = std::string(ipv6sbuf) + ':' + std::to_string(peer6->sin6_port);
    };
  }
  if (store.ss_family==AF_INET) {
    sockaddr_in* peer4 = (sockaddr_in*) &store;
    char ipv4sbuf [INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &peer4->sin_addr, ipv4sbuf, INET_ADDRSTRLEN);
    peer_id = std::string(ipv4sbuf) + ':' + std::to_string(peer4->sin_port);
  }
  return peer_id;
}

