#include "CurlHandler.h"
#include "TorrentFile.h"
#include <string>
#include <vector>

class TrackerManager {
  CurlHandle tracker;
  const TorrentFile& torrent;
  unsigned peer_socket_port;
  const std::string protocol;

  using peer_schema = std::string;
  std::vector<peer_schema> get_peers_http();
  std::vector<peer_schema> get_peers_udp();

public:
  TrackerManager(TorrentFile& torrent_, unsigned psp);
  std::vector<peer_schema> request_peers();
  void update_tracker();
  void scrape_tracker();
};
