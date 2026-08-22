#pragma once
#include "FileManager.hpp"
#include "PeerManager.hpp"
#include "TorrentFile.hpp"
#include "TrackerManager.hpp"

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
