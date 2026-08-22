#pragma once
#include "PeerManagerTypes.hpp"
#include "TorrentFile.hpp"
#include "PeerTransferManager.hpp"
#include "PeerConnectionManager.hpp"
#include <queue>

class PeerManager {
  bool seeding{false};
  TorrentFile& torrent;
  pdisconnection_queue discovered;
  pconnection_queue connects;
  pdisconnection_queue disconnets;
  PeerConnectionManager connection_handler;
  PeerTransferManager transfer_handler;
};
