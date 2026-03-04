#include "CurlHandler.h"
#include "TorrentFile.h"
#include <string>
#include <vector>

class TrackerManager {
  CurlHandle tracker;
  const TorrentFile& torrent;
  unsigned listening_port;
  const std::string protocol;
  std::string info_hash_byte;

  using peer_schema = std::string;
  peer_schema get_peers_udp();
  std::string get_request_params(unsigned uploaded, unsigned downloaded, unsigned left, unsigned compact, std::string event);
  static std::string get_request_url(std::string params);

public:
  peer_schema get_peers_http();
  TrackerManager(TorrentFile& torrent_, unsigned psp);
  std::vector<peer_schema> request_peers();
  void update_tracker();
  void scrape_tracker();
};
