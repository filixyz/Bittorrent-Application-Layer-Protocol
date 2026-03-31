#ifndef TRACKER_MAN
#define TRACKER_MAN

#include "HTTPHandler.h"
#include "UDPHandler.h"
#include "TorrentFile.h"
#include <curl/curl.h>
#include <chrono>
#include <sys/epoll.h>

class TrackerManager {
  class Tracker;
  struct trkr_context_t {
    unsigned uploaded=0; unsigned downloaded=0;
    unsigned left; int compact=1;
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
    protocol_handle_t(int);
  };
  enum class tracker_event:short {
    started=0, stopped=1, completed=2, update=3,
  };
  std::array<std::string, 4> event_strings {
    "&event=started", "&event=stopped", "&event=completed", ""
  };
  struct epoll_handle_t {
    int fd;                           int signal_fd;
    std::vector<epoll_event> events;  int ready_events_size;
    int state_index{-1};
  };

  epoll_handle_t epoll;
  protocol_handle_t protocol;
  const TorrentFile& torrent;
  trkr_context_t tracker_context;
  std::vector<Tracker> tracker_connections;
  protocol_queue_t protocol_queue;
  unsigned listening_port;
  std::string info_hash_byte;
  void (TrackerManager::*current_state)();
  bool running=true;

  void initialize_info_hash_byte();
  void initialize_tracker_context();
  void initiatlize_trackers(std::vector<std::string_view>);
  int  initialize_epoll();
  void initialize_tracker_event_system();

  void handle_events_for_transition();
  void read_state_change_signal();
  void transition_space();

  void start_state();
  void normal_state();
  void reannounce_state();
  void force_reannounce_state();
  void shutdown_state();
  void inactive_state();

  void block_until_ready_events();
  auto do_on_success_cllbk();
  auto do_on_failure_cllbk();
  std::string get_request_params();
  void set_announce_url_for_http_tracker(Tracker&, tracker_event);
  void set_announce_url_for_udp_trackers();
  static int get_retry_seconds(const Tracker*);

  void send_requests();
  void parse_responses();
  void feed_peer_manager();

public:
  TrackerManager(TorrentFile&, unsigned);
  void start_tracker_manager();
  void start();
  void reannounce();
  void force_reannounce();
  void shutdown();
  void update_context(unsigned, unsigned);
  void scrape_trackers();
};

class TrackerManager::Tracker {
  struct network {
    CURL* connection;             network_data data;
  };
  struct time {
    int min_interval{-1};         int interval{-1};
    int min_timer_fd{-1};         int timer_fd{-1};
  };
  struct bools {
    bool is_http_not_udp;         bool active_flag=true;
    bool update_state=false;      bool requeueable=true;
    bool only_one_timer=true;    bool interruptible=true;
  };
  struct data_nest_t {
    network     net_set;          bools       bool_set;
    time        time_set;         std::string tracker_id;
    unsigned    failure_count=0;  unsigned    queue_index;
  };
public:
  const std::string url;
  Tracker(std::string);
  data_nest_t nest;
};

#endif
