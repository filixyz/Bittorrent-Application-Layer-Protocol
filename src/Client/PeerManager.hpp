#pragma once
#include "PeerManagerTypes.h"
#include "TorrentFile.h"
#include "PeerTransferManager.h"
#include "PeerConnectionManager.h"
#include <queue>

class PeerManager {
  bool seeding{false};
  //TorrentFile& torrent;
  pdisconnection_queue discovered;
  pconnection_queue connects;
  pdisconnection_queue disconnets;
  PeerConnectionManager connection_handler;
  PeerTransferManager transfer_handler;
};
