#ifndef CLIENT
#define CLIENT
#include "Torrent_File.h"

class BClient_Instance {
  const Torrent_File &file;
  const std::string magnet_url;
  bool initialization_already_done;
  bool download_complete;
  bool seeding;
  // TrackerManager tracker_man
  // PeerManager peer_man
  // DownloadManager down_man
  // File_Manager file_man

public:
  BClient_Instance(Torrent_File &);
  BClient_Instance() = delete;
  BClient_Instance(const BClient_Instance &) = delete;
  BClient_Instance &operator=(const BClient_Instance &) = delete;
  BClient_Instance(BClient_Instance &&) = delete;
  BClient_Instance &&operator=(BClient_Instance &&) = delete;
  ~BClient_Instance();
  void start() const;
  void pause() const;
  void stop() const;
  void get_stats() const;
  void delete_torrent() const;
};

#endif
