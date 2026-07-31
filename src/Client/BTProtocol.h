#pragma once
#include "TransferManager.h"
#include "FileManager.h"
#include "PeerManager.h"
#include "TorrentFile.h"
#include "TrackerManager.h"

class BTProtocol {
public:
  BTProtocol(TorrentFile &);
  BTProtocol() = delete;
  BTProtocol(const BTProtocol &) = delete;
  BTProtocol &operator=(const BTProtocol &) = delete;
  BTProtocol(BTProtocol &&) = delete;
  BTProtocol &&operator=(BTProtocol &&) = delete;
  ~BTProtocol();
  void start() const;
  void resume() const;
  void pause() const;
  void stop() const;
  void get_stats() const;
  void delete_torrent() const;
};
