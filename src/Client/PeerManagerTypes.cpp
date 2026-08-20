#include "PeerManagerTypes.h"
#include "TCPRingBuffer.h"
#include <sys/types.h>
#include <unistd.h>

ssize_t peer_nonblock_tcp::recv(prepare_t a, transact& b) {
  ephemereal_hdr.msg_iov = a.iovec.first;
  ephemereal_hdr.msg_iovlen = a.iovec.second;
  auto recv_return = recvmsg(socket, &ephemereal_hdr, 0);
  if (recv_return == 0) {
    perrno = PEER_SHUTDOWN;
    b = {false, a.buffered};
    return 0;
  } else if (recv_return<0) {
    b = {handle_errno(errno), a.buffered};
    return 0;
  }
  b = {true, a.buffered};
  return recv_return;
}

transact peer_nonblock_tcp::recv(hanshake_buffer& buffer) {
  transact transaction;
  buffer.commit_write( recv(buffer.prepare_write(), transaction) );
  return transaction;
}

transact peer_nonblock_tcp::recv(session_buffer& buffer) {
  transact transaction;
  buffer.commit_write( recv(buffer.prepare_write(), transaction) );
  return transaction;
}

ssize_t peer_nonblock_tcp::send(prepare_t a, transact& b) {
  ephemereal_hdr.msg_iov = a.iovec.first;
  ephemereal_hdr.msg_iovlen = a.iovec.second;
  auto send_return = sendmsg(socket, &ephemereal_hdr, MSG_NOSIGNAL); // No sigpipe.
  if (send_return<0) {
    b = {handle_errno(errno), a.buffered};
    return 0;
  }
  b = {true, a.buffered};
  return send_return;
}

PeerConnection PeerSession::dummypeer{};

transact peer_nonblock_tcp::send(hanshake_buffer& buffer) {
  transact transaction;
  buffer.commit_read( send(buffer.prepare_read(), transaction) );
  return transaction;
}

transact peer_nonblock_tcp::send(session_buffer& buffer) {
  transact transaction;
  buffer.commit_read( send(buffer.prepare_read(), transaction) );
  return transaction;
}

transact PeerConnection::send() {
  return tcp.send(send_buffer);
}

transact PeerConnection::recv() {
  return tcp.recv(recv_buffer);
}

transact PeerSession::send() {
  return tcp.send(send_buffer);
}

transact PeerSession::recv() {
  return tcp.recv(recv_buffer);
}

void PeerSession::set_endpoint(const PeerConnection* peer_) {
  peer = peer_;
}

bool PeerSession::is_dummy() {
  return peer == &dummypeer;
}

const PeerConnection* PeerSession::endpoint_disconnected() {
  id = 0;
  generation = 0;
  down_rate = 0;
  upld_rate = 0;
  choke.set();
  interest.reset();
  tcp.socket = 0;
  tcp.perrno = 0;
  tcp.ephemereal_hdr.msg_iov = nullptr;
  auto disconnected = peer;
  peer = &dummypeer;
  return disconnected;
}
