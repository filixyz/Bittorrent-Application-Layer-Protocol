#ifndef TRACKER_MAN
#define TRACKER_MAN

#include "HTTPHandler.h"
#include "UDPHandler.h"
#include "TorrentFile.h"
#include <curl/curl.h>
#include <chrono>

class Tracker {
  struct network {
    CURL* connection;
    network_data data;
  };
  struct time {
    int min_interval{-1};
    int interval{-1};
    int min_timer_fd;
    int timer_fd;
  };
  struct bools {
    // protocol flag
    bool is_http_not_udp;
    // tracker server state flags
    bool active_flag=true; bool update_state=false;
    // shutdown determining flag
    bool requeueable=true;
    // timer description flag
    bool only_one_timer=false; bool interruptible=true;
  };
public:
  Tracker(std::string);
  const std::string url;

  bools bool_set;
  network net_set;
  time time_set;

  std::string tracker_id;
  int queue_index=-1;
  unsigned failure_count=0;
};

enum class trkr_manager_state {
  normal, reannounce, force_reannounce, shutdown
};

class TrackerManager {
  struct trkr_context_t {
    unsigned uploaded=0; unsigned downloaded=0;
    unsigned left; int compact=1;
    trkr_manager_state state= trkr_manager_state::normal;
  };
  struct protocol_queue_t {
    std::vector<Tracker*> http;
    std::vector<Tracker*> udp;
    void populate(std::vector<Tracker>&);
    void enqueue(Tracker*);
    void dequeue(Tracker*);
  };
  struct protocol_handle_t {
    HTTPHandler http;
    UDPHandler udp;
    protocol_handle_t() = default;
  };
  enum class tracker_event:short {
    started=0, stopped=1, completed=2, update=3,
  };
  std::array<std::string, 4> event_strings {
    "&event=started", "&event=stopped", "&event=completed", ""
  };

  int epoll_fd;
  protocol_handle_t protocol;
  const TorrentFile& torrent;
  trkr_context_t tracker_context;
  std::vector<Tracker> tracker_connections;
  protocol_queue_t protocol_queue;
  unsigned listening_port;
  std::string info_hash_byte;

  std::string get_request_params();
  void initiatlize_trackers(std::vector<std::string_view>);
  void set_announce_url_for_http_tracker(Tracker&, tracker_event);
  void queue_in_http_requests_auto();
  void set_announce_url_for_udp_trackers();
  void queue_in_udp_requests_auto();
  void send_requests();
  void parse_responses();
  void feed_peer_manager();

public:
  TrackerManager(TorrentFile&, unsigned);
  void announce();
  void reannounce();
  void force_reannounce();
  void shutdown();
  void update_context(unsigned, unsigned);
  void scrape_trackers();
};

#endif
