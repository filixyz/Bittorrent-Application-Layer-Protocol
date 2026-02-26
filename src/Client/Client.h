#ifndef CLIENT
#define CLIENT
#include "DownloadManager.h"
#include "FileManager.h"
#include "PeerManager.h"
#include "TorrentFile.h"
#include "TrackerManager.h"

class BClient_Instance {
  const TorrentFile &file;
  const std::string magnet_url;
  bool initialization_already_done;
  bool download_complete;
  bool seeding;
  TrackerManager tracker_man;
  PeerManager peer_man;
  DownloadManager down_man;
  FileManager file_man;

public:
  BClient_Instance(TorrentFile &);
  BClient_Instance() = delete;
  BClient_Instance(const BClient_Instance &) = delete;
  BClient_Instance &operator=(const BClient_Instance &) = delete;
  BClient_Instance(BClient_Instance &&) = delete;
  BClient_Instance &&operator=(BClient_Instance &&) = delete;
  ~BClient_Instance();
  void start() const;
  void resume() const;
  void pause() const;
  void stop() const;
  void get_stats() const;
  void delete_torrent() const;
};

#endif
