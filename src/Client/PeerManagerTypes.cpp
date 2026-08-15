#include "PeerManagerTypes.h"

std::pair<bool, bool> PeerConnection::recv() {
  auto [io_vec, io_possible] = recv_buffer.prepare_write();
  ephemereal_hdr.msg_iov = io_vec.first;
  ephemereal_hdr.msg_iovlen = io_vec.second;
  auto recv_return = recvmsg(socket, &ephemereal_hdr, 0);
  if (recv_return == 0) {
    disconnect();
    return {false, io_possible};
  } else if (recv_return<0) {
    return {handle_errno(errno), io_possible};
  }
  recv_buffer.commit_write(recv_return);
  return {true, io_possible};
}

std::pair<bool, bool> PeerConnection::send() {
  auto [io_vec, io_possible] = send_buffer.prepare_read();
  ephemereal_hdr.msg_iov = io_vec.first;
  ephemereal_hdr.msg_iovlen = io_vec.second;
  auto send_return = sendmsg(socket, &ephemereal_hdr, MSG_NOSIGNAL); // No sigpipe.
  if (send_return<0) {
    return {handle_errno(errno), io_possible};
  }
  send_buffer.commit_read(send_return);
  return {true, io_possible};
}

PeerConnection::PeerConnection(std::string a, std::size_t b): peer_id(a), bitfield(b) {
  ephemereal_hdr.msg_name = nullptr;
  ephemereal_hdr.msg_control = nullptr;
}

PeerConnection PeerSession::dummypeer{"dummy", 0};

void PeerSession::set_endpoint(PeerConnection& peer_) {
  peer = &peer_;
}

bool PeerSession::is_dummy() {
  return peer == &dummypeer;
}

void PeerSession::endpoint_disconnected() {
  id = 0;
  generation = 0;
  down_rate = 0;
  upld_rate = 0;
  choke.set();
  interest.reset();
  peer = &dummypeer;
}
