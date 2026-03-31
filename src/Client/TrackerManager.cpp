#include "TrackerManager.h"
#include "Constants.h"
#include "HTTPHandler.h"
#include "Hasher.h"
#include <array>
#include <cstdlib>
#include <ctime>
#include <curl/curl.h>
#include <curl/multi.h>
#include <string>
#include <sys/time.h>
#include <utility>
#include <vector>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <sys/eventfd.h>

TrackerManager::Tracker::Tracker(std::string url_) : url(url_) {};

TrackerManager::protocol_handle_t::protocol_handle_t(int epfd) : http(epfd) {}

void TrackerManager::protocol_queue_t::populate(std::vector<Tracker>& trackers) {
  int http_index=0; int udp_index=0;
  for(Tracker& trkr: trackers) {
    if(trkr.nest.bool_set.is_http_not_udp) {
      http[http_index] = &trkr;
      trkr.nest.queue_index = http_index;
    } else {
      udp[udp_index] = &trkr;
      trkr.nest.queue_index = udp_index;
    }
  }
  http.shrink_to_fit(); udp.shrink_to_fit();
}
void TrackerManager::protocol_queue_t::enqueue(Tracker* trkr) {
  http[trkr->nest.queue_index] = trkr;
}
void TrackerManager::protocol_queue_t::dequeue(Tracker* trkr) {
  http[trkr->nest.queue_index] = nullptr;
}

void TrackerManager::initiatlize_trackers(std::vector<std::string_view> trackers_urls) {
  for(auto url : trackers_urls) {
    Tracker current{std::string(url)};
    current.nest.bool_set.is_http_not_udp = url.starts_with("http") ? true : false;
    current.nest.net_set.connection = current.nest.bool_set.is_http_not_udp ? HTTPHandler::new_easy(current.nest.net_set.data) : nullptr;
    tracker_connections.push_back(std::move(current));
  }
  tracker_connections.shrink_to_fit();
}
void TrackerManager::initialize_info_hash_byte() {
  std::string_view info_key = torrent.get_info_key();
  const std::span<const std::byte> hash_byte_view(reinterpret_cast<const std::byte*>(info_key.data()), info_key.size());
  const std::vector<std::byte> info_hash_bytes = Hasher::get_sha1(hash_byte_view);
  info_hash_byte = Hasher::byte_stringify_hash(info_hash_bytes);
}
void TrackerManager::initialize_tracker_context() {
  tracker_context.left = torrent.get_download_size();
}

int TrackerManager::initialize_epoll() {
  epoll.fd = epoll_create1(EPOLL_CLOEXEC);
  return epoll.fd;
}

void TrackerManager::initialize_tracker_event_system() {
  constexpr int max_tracker_events_per_tracker=2;               // two timer intervals min and normal
  constexpr int max_protocol_events_per_tracker=1;              // events related to a http or udp connection
  constexpr int max_number_of_network_driver_signals=1;         // the timer fd, curl multi uses to drive timeouts
  constexpr int max_number_of_tracker_state_change_signals=1;   // the epoll.signal_fd tracker manager uses to intercept state changes
  epoll.events.reserve (
    tracker_connections.size()
    * (max_tracker_events_per_tracker + max_protocol_events_per_tracker)
    + max_number_of_network_driver_signals
    + max_number_of_tracker_state_change_signals
  );
  while((epoll.signal_fd = eventfd(0, EFD_CLOEXEC|EFD_NONBLOCK))==-1);
  struct epoll_event event { .events=EPOLLIN, .data.fd=epoll.signal_fd };
  while((epoll_ctl(epoll.fd, EPOLL_CTL_ADD, epoll.signal_fd, &event))==-1);
}

TrackerManager::TrackerManager(TorrentFile& torrent_, unsigned psp_)
  : protocol(initialize_epoll()), torrent(torrent_), listening_port(psp_) {
  initialize_info_hash_byte();
  initialize_tracker_context();
  initiatlize_trackers(torrent.get_tracker_urls());
  initialize_tracker_event_system();
}

std::string TrackerManager::get_request_params()
{
  auto make_param = [](std::string_view key, std::string_view value) {
    return std::string(key).append(1, '=').append(value);
  };
  std::string info_hash_byte_escaped_copy = info_hash_byte;
  HTTPHandler::escape_byte_string(info_hash_byte_escaped_copy);
  std::array<std::string, 8> params{
    make_param("info_hash", info_hash_byte_escaped_copy),
    make_param("peer_id", client::constants::client_id),
    make_param("port", std::to_string(listening_port)),
    make_param("uploaded", std::to_string(tracker_context.uploaded)),
    make_param("downloaded", std::to_string(tracker_context.downloaded)),
    make_param("left", std::to_string(tracker_context.left)),
    make_param("compact", std::to_string(tracker_context.compact)),
  };
  auto accumulate_params = [ampersand='&'](std::array<std::string, 8> params) {
    std::string loaded_paramaters;
    for(std::string& param : params) loaded_paramaters += std::move(param) + ampersand;
    loaded_paramaters.pop_back();
    return loaded_paramaters;
  };
  return accumulate_params(params);
}

void TrackerManager::set_announce_url_for_http_tracker(Tracker& trkr, tracker_event event) {
  if (!trkr.nest.bool_set.is_http_not_udp)
    return;
  std::string tracker_id = trkr.nest.tracker_id.empty() ? "" : std::string{'&'}.append("trackerid=").append(trkr.nest.tracker_id);
  std::string request_url = trkr.url + '?' + get_request_params() + tracker_id + event_strings[static_cast<short>(event)];
  curl_easy_setopt(trkr.nest.net_set.connection, CURLOPT_URL, request_url.data());
}

int TrackerManager::get_retry_seconds(const Tracker* trkr) {
  constexpr int minute = 60;
  std::array<int, 5> retry_intervals {10, 30, 5*minute, 15*minute, 45*minute};
  if(trkr->nest.failure_count==0)
    return 0;
  return retry_intervals[trkr->nest.failure_count-1 %5];
}

void arm_timerfd(int tfd, int seconds, int nanoseconds) {
  struct itimerspec arm { .it_interval{0, 0}, .it_value{seconds, nanoseconds}};
  while(timerfd_settime(tfd, TFD_TIMER_ABSTIME, &arm, nullptr)==-1);
}
void disarm_timerfd(int tfd) {
  struct itimerspec disarm {{0, 0}, {0, 0}};
  while(timerfd_settime(tfd, TFD_TIMER_ABSTIME, &disarm, nullptr)==-1);
}

auto TrackerManager::do_on_success_cllbk() {
  return [this](void* tracker) {

  auto trkr = reinterpret_cast<Tracker*>(tracker);
  trkr->nest.failure_count=0;
  trkr->nest.bool_set.active_flag=true;
  trkr->nest.bool_set.update_state=true;

  // place bendecode parse and tracker timer updates here
  // send peers to peermanager here

  if(trkr->nest.time_set.min_interval==0 || trkr->nest.time_set.min_interval==-1)
    trkr->nest.bool_set.only_one_timer=true;

  if(!trkr->nest.bool_set.only_one_timer) {
    if(trkr->nest.time_set.min_timer_fd==-1) {
      struct epoll_event current_event { .events=EPOLLIN, .data{trkr} };
      while ((trkr->nest.time_set.min_timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC|TFD_NONBLOCK))==-1);
      while (epoll_ctl(epoll.fd, EPOLL_CTL_ADD, trkr->nest.time_set.min_timer_fd, &current_event)==-1);
    }
    arm_timerfd(trkr->nest.time_set.min_timer_fd, trkr->nest.time_set.min_interval, 0);
    trkr->nest.bool_set.interruptible=false;
  }
  arm_timerfd(trkr->nest.time_set.interval, trkr->nest.time_set.interval, 0);

  if (trkr->nest.bool_set.requeueable)
    protocol_queue.enqueue(trkr);

  };
}

auto TrackerManager::do_on_failure_cllbk() {
  return [this](void* tracker) {

  auto trkr = reinterpret_cast<Tracker*>(tracker);
  trkr->nest.failure_count++;
  trkr->nest.bool_set.active_flag=false;
  int secs = trkr->nest.time_set.interval = get_retry_seconds(trkr);
  arm_timerfd(trkr->nest.time_set.timer_fd, secs, 0);
  trkr->nest.bool_set.only_one_timer=true;
  if (trkr->nest.bool_set.requeueable)
    protocol_queue.enqueue(trkr);

  };
}

int create_timer_fd() {
  int fd;
  while ((fd =timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC|TFD_NONBLOCK))==-1);
  return fd;
}

void register_fd_to_epoll(int epoll_fd, int fd, void* trkr) {
  struct epoll_event current_event { .events=EPOLLIN, .data=trkr };
  while (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &current_event)==-1);
}

void read_timout() {
}
void read_event() {
}

void TrackerManager::handle_events_for_transition() {
  for(int index=0; index<epoll.ready_events_size; ++index) {

    if(epoll.events[index].data.fd == epoll.signal_fd) {
      epoll.state_index=index;
      continue;
    }

    // need to devise a way to know when an epoll.events[index].data.ptr is either a tracker object or a request object


    // read event timeout here
    // read event timeout here
    // read event timeout here
    Tracker* trkr = reinterpret_cast<Tracker*>(epoll.events[index].data.ptr);
    if(!trkr->nest.bool_set.only_one_timer && !trkr->nest.bool_set.interruptible) {
      trkr->nest.bool_set.interruptible=true;
      continue;
    }
    protocol_queue.dequeue(trkr);
    if(!trkr->nest.bool_set.update_state)
      set_announce_url_for_http_tracker(*trkr, tracker_event::started);
    else
      set_announce_url_for_http_tracker(*trkr, tracker_event::update);
    request_t current_request {trkr->nest.net_set.connection, trkr, do_on_success_cllbk(), do_on_failure_cllbk()};
    protocol.http.add_request(current_request);
  }
}

void TrackerManager::read_state_change_signal(){
}

void TrackerManager::transition_space() {
  handle_events_for_transition();
  read_state_change_signal();
}

void TrackerManager::start_state() {
  transition_space();
  protocol_queue.populate(tracker_connections);
  for(auto trkr : protocol_queue.http) {
    if(trkr->nest.time_set.interval==-1 && trkr->nest.time_set.min_interval==-1) { // first time tracker will be queued
      trkr->nest.bool_set.only_one_timer = true;
      trkr->nest.time_set.interval=0;
      trkr->nest.time_set.timer_fd = create_timer_fd();
      register_fd_to_epoll(epoll.fd, trkr->nest.time_set.timer_fd, trkr);
      arm_timerfd(trkr->nest.time_set.timer_fd, 0, 1);  // timer initialized to 1 nanosec so it expires quickly
      continue;
    }
    arm_timerfd(trkr->nest.time_set.timer_fd, trkr->nest.time_set.interval, 0);
    if(trkr->nest.bool_set.only_one_timer==false)
      arm_timerfd(trkr->nest.time_set.min_timer_fd, trkr->nest.time_set.min_interval, 0);
  }
  current_state=&TrackerManager::normal_state;
}

void TrackerManager::normal_state() {
  handle_events_for_transition();
}

void TrackerManager::reannounce_state() {
  transition_space();
  for(Tracker* trkr : protocol_queue.http) {
    if (trkr != nullptr && !trkr->nest.bool_set.only_one_timer && trkr->nest.bool_set.interruptible && trkr->nest.bool_set.update_state) {
      // since we are doing what the normal interval would had done we should disarm the main
      // interval timer. so epoll doesnt wake on it
      disarm_timerfd(trkr->nest.time_set.timer_fd);
      // then handle non interrupt reannounce
      protocol_queue.dequeue(trkr);
      set_announce_url_for_http_tracker(*trkr, tracker_event::update);
      request_t current_request {trkr->nest.net_set.connection, trkr, do_on_success_cllbk(), do_on_failure_cllbk()};
      protocol.http.add_request(current_request);
    }
  }
  current_state=&TrackerManager::normal_state;
}

void TrackerManager::force_reannounce_state() {
  transition_space();
  for(Tracker* trkr : protocol_queue.http) {
    if(trkr != nullptr && trkr->nest.bool_set.update_state) {
      // disarm normal both timers;
      disarm_timerfd(trkr->nest.time_set.timer_fd);
      disarm_timerfd(trkr->nest.time_set.min_timer_fd);
      // then handle non interrupt reannounce
      protocol_queue.dequeue(trkr);
      set_announce_url_for_http_tracker(*trkr, tracker_event::update);
      request_t current_request {trkr->nest.net_set.connection, trkr, do_on_success_cllbk(), do_on_failure_cllbk()};
      protocol.http.add_request(current_request);
    }
  }
  current_state=&TrackerManager::normal_state;
}

void TrackerManager::shutdown_state() {
  transition_space();
  for(Tracker* trkr : protocol_queue.http) {
    if (trkr == nullptr)
      continue;
    // disarm normal both timers;
    // then handle the handle interrupt
    if(!trkr->nest.bool_set.update_state) {
      disarm_timerfd(trkr->nest.time_set.timer_fd);
      trkr->nest.bool_set.requeueable=false;
      protocol_queue.dequeue(trkr);
      continue;
    }
    if(!trkr->nest.bool_set.only_one_timer)
      disarm_timerfd(trkr->nest.time_set.min_timer_fd);
    disarm_timerfd(trkr->nest.time_set.timer_fd);
    protocol_queue.dequeue(trkr);
    tracker_event current_ev = tracker_context.left==0 ? tracker_event::completed : tracker_event::stopped;
    set_announce_url_for_http_tracker(*trkr, current_ev);
    trkr->nest.bool_set.requeueable=false;
    request_t current_request {trkr->nest.net_set.connection, trkr, do_on_success_cllbk(), do_on_failure_cllbk()};
    protocol.http.add_request(current_request);
  }
  // implement condition that should hold before tracker can then
  // transition into an inactive state
}

void TrackerManager::inactive_state() {
}

void TrackerManager::block_until_ready_events() {
  epoll.ready_events_size = epoll_wait(epoll.fd, epoll.events.data(), epoll.events.capacity(), -1);
}

void TrackerManager::start_tracker_manager() {
  current_state = &TrackerManager::inactive_state;
  while(running) {
    block_until_ready_events();
    protocol.http.drive();
    (this->*current_state)();
  }
};

void TrackerManager::start() {
  if(current_state!=&TrackerManager::inactive_state)
    return;
  current_state=&TrackerManager::start_state;
  // trigger state change signal via eventfd here
}
void TrackerManager::reannounce() {
  if(current_state!=&TrackerManager::normal_state)
    return;
  current_state=&TrackerManager::reannounce_state;
  // trigger state change signal via eventfd here
}
void TrackerManager::force_reannounce() {
  if(current_state!=&TrackerManager::normal_state)
    return;
  current_state=&TrackerManager::force_reannounce_state;
  // trigger state change signal via eventfd here
}
void TrackerManager::shutdown() {
  if(current_state!=&TrackerManager::normal_state)
    return;
  current_state=&TrackerManager::shutdown_state;
  // trigger state change signal via eventfd here
}

void TrackerManager::update_context(unsigned dwn, unsigned upd) {
  tracker_context.downloaded += dwn;
  tracker_context.uploaded   += upd;
  tracker_context.left       -= dwn;
}

void TrackerManager::set_announce_url_for_udp_trackers() {}
