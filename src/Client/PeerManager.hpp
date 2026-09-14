#pragma once
#include "PeerManagerTypes.hpp"
#include "TorrentFile.hpp"
#include "PeerTransferManager.hpp"
#include "PeerConnectionManager.hpp"

class PeerManager {
  bool seeding{false};
  TorrentFile& torrent;
  PeerConnectionManager connection_handler;
  PeerTransferManager transfer_handler;
};
