#ifndef CLIENT
#define CLIENT
#include "TransferManager.h"
#include "FileManager.h"
#include "PeerManager.h"
#include "TorrentFile.h"
#include "TrackerManager.h"

class Client {

public:
  Client(TorrentFile &);
  Client() = delete;
  Client(const Client &) = delete;
  Client &operator=(const Client &) = delete;
  Client(Client &&) = delete;
  Client &&operator=(Client &&) = delete;
  ~Client();
  void start() const;
  void resume() const;
  void pause() const;
  void stop() const;
  void get_stats() const;
  void delete_torrent() const;
};

#endif
