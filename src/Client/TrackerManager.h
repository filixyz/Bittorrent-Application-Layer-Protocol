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
    std::chrono::seconds min_interval{-1};
    std::chrono::seconds interval{-1};
    std::chrono::time_point<std::chrono::steady_clock>
      last_transaction_tp = std::chrono::steady_clock::now();
  };
public:
  Tracker(std::string);
  const std::string url;
  std::string tracker_id;
  network net_d;
  time time_d;
};

enum class tracker_event:short {
  started=0, stopped=1, completed=2, update=3
};

class TrackerManager {
  HTTPHandler http;
  const TorrentFile& torrent;
  struct trkr_context_t {
    unsigned uploaded=0; unsigned downloaded=0;
    unsigned left; int compact=1;
    std::string event="start";
  };
  trkr_context_t tracker_context;
  std::vector<Tracker> tracker_connections;
  unsigned listening_port;
  std::string info_hash_byte;
  std::array<std::string, 4> event_strings {
    "&event=started", "&event=stopped", "&event=completed", ""
  };

  std::string get_request_params();
  void initiatlize_trackers(std::vector<std::string_view>);
  void set_announce_url_for_http_trackers(tracker_event);
  void set_announce_url_for_udp_trackers();
  void queue_in_udp_requests();
  void queue_in_http_requests(tracker_event);
  void send_requests();
  void parse_responses();
  void feed_peer_manager();

public:
  TrackerManager(TorrentFile&, unsigned);
  void announce();
  void announce(tracker_event);
  void update_context();
  void scrape_trackers();
};
#endif
