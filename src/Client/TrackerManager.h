#ifndef TRACKER_MAN
#define TRACKER_MAN

#include "HTTPHandler.h"
#include "TorrentFile.h"
#include <curl/curl.h>
#include <chrono>

struct Tracker {
private:
  struct network {
    CURL* connection;
    network_data data;
    bool is_http_not_udp;
  };
  struct time {
    std::chrono::seconds min_interval{0};
    std::chrono::seconds interval{0};
    std::chrono::time_point<std::chrono::steady_clock>
      last_transaction_tp = std::chrono::steady_clock::now();
  };
public:
  std::string tracker_id;
  network net_d;
  time time_d;
};

class TrackerManager {
  HTTPHandler http;
  const TorrentFile& torrent;
  std::vector<Tracker> tracker_connections;
  unsigned listening_port;
  std::string info_hash_byte;

  using peer_schema = std::string;
  peer_schema get_peers_udp();
  peer_schema get_peers_http();
  std::string get_request_params(unsigned uploaded, unsigned downloaded, unsigned left, unsigned compact, std::string event);
  static std::string get_request_url(std::string params);
  void initiatlize_trackers(std::vector<std::string_view>);
  void partition_initialized_trackers();

public:
  TrackerManager(TorrentFile& torrent_, unsigned psp);
  void request_peers();
  void update_tracker();
  void scrape_tracker();
};
#endif
