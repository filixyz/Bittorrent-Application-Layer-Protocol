#include "PeerManagerTypes.hpp"
#include <cerrno>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

bool peer_nonblock_tcp::pconnect(const sockaddr* addr) {
  if (socket == -1) {
    perrno = EBADF;
    return false;
  }
  socklen_t addrlen;
  if      (addr->sa_family == AF_INET)  addrlen = sizeof(sockaddr_in);
  else if (addr->sa_family == AF_INET6) addrlen = sizeof(sockaddr_in6);
  else    {
    perrno = EAFNOSUPPORT;
    return false;
  }
  int connect_return = connect(socket, addr, addrlen);
  if (connect_return<0)
    return handle_connect_perrno(errno);
  perrno = -1;
  socket = connect_return;
  return true;
}

bool peer_nonblock_tcp::handle_send_perrno(int err) {
  perrno = err;
  switch (perrno) {
    case EAGAIN:
      return true;
    case EPIPE:
    case ECONNRESET:
    default:
      return false;
  }
}

bool peer_nonblock_tcp::handle_recv_perrno(int err) {
  perrno = err;
  switch (perrno) {
    case EAGAIN:
      return true;
    default:
      return false;
  }
}

bool peer_nonblock_tcp::handle_connect_perrno(int err) {
  perrno = err;
  switch (perrno) {
    case EINPROGRESS:
      return true;
    default:
      return false;
  }
}

void peer_nonblock_tcp::pclose() {
  if (socket == -1) {
    perrno = EBADF;
    return;
  }
  close(socket);
  socket = -1;
  perrno = -1;
}

void peer_nonblock_tcp::disconnect(){
  pclose();
}

transact PeerConnection::send() {
  return tcp.send(send_buffer);
}

transact PeerConnection::recv() {
  return tcp.recv(recv_buffer);
}

bool PeerConnection::connect() {
  return tcp.pconnect((sockaddr*)&store);
}

PeerConnection PeerSession::dummypeer{};

transact PeerSession::send() {
  return tcp.send(send_buffer);
}

transact PeerSession::recv() {
  return tcp.recv(recv_buffer);
}

bool PeerSession::is_dummy() {
  return peer == &dummypeer;
}

void PeerSession::set_endpoint(const connect_update endpoint) {
  peer = endpoint.peer;
  id = endpoint.id;
  generation = endpoint.generation;
  tcp.socket = endpoint.socket;
}

disconnect_update PeerSession::endpoint_disconnected() {
  disconnect_update disconnected {
    .peer=peer, .generation=generation, .perrno=tcp.perrno
  };
  id = 0;
  generation = 0;
  down_rate = 0;
  upld_rate = 0;
  choke.set();
  interest.reset();
  close(tcp.socket);
  tcp.socket = -1;
  tcp.perrno = -1;
  tcp.ephemereal_hdr.msg_iov = nullptr;
  tcp.ephemereal_hdr.msg_iovlen = 0;
  recv_buffer.reset();
  send_buffer.reset();
  watcher.for_sock.stop();
  watcher.for_timer.stop();
  peer = &dummypeer;
  return disconnected;
}
