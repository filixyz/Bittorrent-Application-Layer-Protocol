#include "CurlHandler.h"
#include "TorrentFile.h"
#include <string>
#include <vector>

class TrackerManager {
  CurlHandle tracker;
  const TorrentFile& torrent;
  unsigned listening_port;
  const std::string protocol;

  using peer_schema = std::string;
  peer_schema get_peers_http();
  peer_schema get_peers_udp();

public:
  TrackerManager(TorrentFile& torrent_, unsigned psp);
  std::vector<peer_schema> request_peers();
  void update_tracker();
  void scrape_tracker();
};
