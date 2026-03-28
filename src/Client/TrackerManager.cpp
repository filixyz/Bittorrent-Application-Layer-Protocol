#include "TrackerManager.h"
#include "Constants.h"
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

Tracker::Tracker(std::string url_) : url(url_) {};

void TrackerManager::protocol_queue_t::populate(std::vector<Tracker>& trackers) {
  int http_index=0; int udp_index=0;
  for(Tracker& trkr: trackers) {
    if(trkr.bool_set.is_http_not_udp) {
      http[http_index] = &trkr;
      trkr.queue_index = http_index++;
    } else {
      udp[udp_index] = &trkr;
      trkr.queue_index = udp_index++;
    }
  }
  http.shrink_to_fit(); udp.shrink_to_fit();
}

void TrackerManager::protocol_queue_t::enqueue(Tracker* trkr) {
  if(trkr->bool_set.is_http_not_udp) http[trkr->queue_index] = trkr;
  else udp[trkr->queue_index] = trkr;
}

void TrackerManager::protocol_queue_t::dequeue(Tracker* trkr) {
  if(trkr->bool_set.is_http_not_udp) http[trkr->queue_index] = nullptr;
  else udp[trkr->queue_index] = nullptr;
}

void TrackerManager::initiatlize_trackers(std::vector<std::string_view> trackers_urls) {
  for(auto url : trackers_urls) {
    Tracker current{std::string(url)};
    current.bool_set.is_http_not_udp = url.starts_with("http") ? true : false;
    current.net_set.connection = current.bool_set.is_http_not_udp ? HTTPHandler::new_easy(current.net_set.data) : nullptr;
    tracker_connections.push_back(std::move(current));
  }
  tracker_connections.shrink_to_fit();
}

TrackerManager::TrackerManager(TorrentFile& torrent_, unsigned psp_)
  : torrent(torrent_), listening_port(psp_) {
    /* info hash byte initialization */
    std::string_view info_key = torrent.get_info_key();
    const std::span<const std::byte> hash_byte_view(reinterpret_cast<const std::byte*>(info_key.data()), info_key.size());
    const std::vector<std::byte> info_hash_bytes = Hasher::get_sha1(hash_byte_view);
    info_hash_byte = Hasher::byte_stringify_hash(info_hash_bytes);
    /* info hash byte initialization ends here */
    tracker_context.left = torrent.get_download_size();
    initiatlize_trackers(torrent.get_tracker_urls());
    protocol_queue.populate(tracker_connections);
    epoll_fd = epoll_create1(EPOLL_CLOEXEC);
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
    if (!trkr.bool_set.is_http_not_udp)
      return;
    std::string tracker_id = trkr.tracker_id.empty() ? "" : std::string{'&'}.append("trackerid=").append(trkr.tracker_id);
    std::string request_url = trkr.url + '?' + get_request_params() + tracker_id + event_strings[static_cast<short>(event)];
    curl_easy_setopt(trkr.net_set.connection, CURLOPT_URL, request_url.data());
}

int get_retry_seconds(const Tracker* trkr) {
  constexpr int minute = 60;
  std::array<int, 5> retry_intervals {10, 30, 5*minute, 15*minute, 45*minute};
  if(trkr->failure_count==0)
    return 0;
  return retry_intervals[trkr->failure_count-1 %5];
}

void do_on_success(void * tracker) {
  // set tracker.active to true
  // set tracker.update to true
  // parse response, update tracker intervals, send peers to peermanager,
  // if enqueue flag is true:
  //  determine if one timer tracker or two timer tracker
  //  depending on which initialize that fitting timer specifications
  //  then enqueue back
}

void do_on_failure(void * tracker) {
  // set tracker.active to false
  // if enqueue flag is true:
  //  will be one timer tracker so initialize its timer specifications
  //  enqueue back
}

void disarm_timerfd(int tfd) {
  struct itimerspec disarm {{0, 0}, {0, 0}};
  while(timerfd_settime(tfd, TFD_TIMER_ABSTIME, &disarm, nullptr)==-1);
}

void TrackerManager::queue_in_http_requests_auto() {
  //---------- INITIALIZE TIMER SPECIFICATIONS FOR TRACKERS THE PROTOCOL HANDLER JUST HANDLED---------//
  for (auto trkr : protocol_queue.http) {
    if(trkr==nullptr)
      continue;
    struct itimerspec intervals;
    if(trkr->time_set.interval == -1 && trkr->time_set.min_interval ==-1) {
      // This emans this is the first time this tracker is being queued
      // so we create a new timer object for it that expires quickly;
      // initialize timer with 1 nanosec
      while ((trkr->time_set.min_timer_fd =timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC|TFD_NONBLOCK))==-1);
      while ((trkr->time_set.timer_fd     =timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC|TFD_NONBLOCK))==-1);

      struct epoll_event current_event { .events=EPOLLIN, .data=trkr };
      while (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, trkr->time_set.min_timer_fd, &current_event)==-1) ;
      while (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, trkr->time_set.timer_fd, &current_event)==-1);

      intervals = { .it_interval{0, 0}, .it_value{.tv_sec=0, .tv_nsec=1} };
      while((timerfd_settime(trkr->time_set.min_timer_fd, TFD_TIMER_ABSTIME, &intervals, nullptr))==-1);
      trkr->time_set.min_interval = 0;
      trkr->bool_set.only_one_timer=true;
    }
    else if(!trkr->bool_set.active_flag) {
      // This means that this tracker request just failed failed
      // so here a comfortable time is set for re-entry
      // This would also suffice for cases where the tracker has been in perpetual failure
      trkr->failure_count++;
      intervals = { .it_interval{0, 0}, .it_value{get_retry_seconds(trkr), 0}};
      while((timerfd_settime(trkr->time_set.min_timer_fd, TFD_TIMER_ABSTIME, &intervals, nullptr))==-1);
      trkr->bool_set.only_one_timer=true;
    }
    else {
      // timer has been initialized before and requests had been successful
      // so we can use its responded interval to set timeout,
      trkr->failure_count=0; // Reset failure count in case this tracker just recovered from an inactive state.
      intervals = { .it_interval{0, 0}, .it_value{.tv_sec=trkr->time_set.min_interval, .tv_nsec=0} };
      while((timerfd_settime(trkr->time_set.min_timer_fd, TFD_TIMER_ABSTIME, &intervals, nullptr))==-1);
      if(trkr->time_set.interval==-1 || trkr->time_set.interval==0) { //  meaning only one interval reponse
        trkr->bool_set.only_one_timer=true;
      } else {
        trkr->bool_set.only_one_timer=false;

        intervals = { .it_interval{0, 0}, .it_value{.tv_sec=trkr->time_set.interval,     .tv_nsec=0} };
        while((timerfd_settime(trkr->time_set.timer_fd, TFD_TIMER_ABSTIME, &intervals, nullptr))==-1);
        trkr->bool_set.interruptible =false;
      }
    }
  }
  //------------------------- TIMER SPECIFICATIONS INITIALIZATION ENDS HERE --------------------------//
  // handles tracker in normal state
  if (tracker_context.state == trkr_manager_state::normal) {
    constexpr int SOME_VALUE=100;
    std::array<struct epoll_event, SOME_VALUE> events;
    int n = epoll_wait(epoll_fd, events.data(), SOME_VALUE, -1);
    for(int index=0; index<n; ++index) {
    // read event timeout here
      Tracker* trkr = reinterpret_cast<Tracker*>(events[index].data.ptr);

      if(!trkr->bool_set.only_one_timer && !trkr->bool_set.interruptible) {
        trkr->bool_set.interruptible=true;
        continue;
      }
      protocol_queue.dequeue(trkr);
      if(!trkr->bool_set.update_state)
        set_announce_url_for_http_tracker(*trkr, tracker_event::started);
      else
        set_announce_url_for_http_tracker(*trkr, tracker_event::update);
      request_t current_request {trkr->net_set.connection, trkr, do_on_success, do_on_failure};
      protocol.http.add_request(current_request);
    }
  }
  // handles re-announcement
  if(tracker_context.state == trkr_manager_state::reannounce) {
    for(Tracker* trkr : protocol_queue.http) {
      if (trkr != nullptr && !trkr->bool_set.only_one_timer && trkr->bool_set.interruptible && trkr->bool_set.update_state) {
        // since we are doing what the normal interval would had done we should disarm the main
        // interval timer. so epoll doesnt wake on it
        disarm_timerfd(trkr->time_set.timer_fd);
        // then handle non interrupt reannounce
        protocol_queue.dequeue(trkr);
        set_announce_url_for_http_tracker(*trkr, tracker_event::update);
        request_t current_request {trkr->net_set.connection, trkr, do_on_success, do_on_failure};
        protocol.http.add_request(current_request);
      }
    }
    tracker_context.state = trkr_manager_state::normal;
  }

  // handles force-re-announcement
  if (tracker_context.state == trkr_manager_state::force_reannounce) {
    for(Tracker* trkr : protocol_queue.http) {
      if(trkr != nullptr && trkr->bool_set.update_state) {
        // disarm normal both timers;
        disarm_timerfd(trkr->time_set.timer_fd);
        disarm_timerfd(trkr->time_set.min_timer_fd);
        // then handle non interrupt reannounce
        protocol_queue.dequeue(trkr);
        set_announce_url_for_http_tracker(*trkr, tracker_event::update);
        request_t current_request {trkr->net_set.connection, trkr, do_on_success, do_on_failure};
        protocol.http.add_request(current_request);
      }
    }
    tracker_context.state = trkr_manager_state::normal;
  }

  // handles termination event like shutdown or completed
  if(tracker_context.state == trkr_manager_state::shutdown) {
    for(Tracker* trkr : protocol_queue.http) {
      if (trkr == nullptr)
        continue;
      // disarm normal both timers;
      // then handle the handle interrupt
      if(!trkr->bool_set.update_state) {
        disarm_timerfd(trkr->time_set.min_timer_fd);
        trkr->bool_set.requeueable=false;
        protocol_queue.dequeue(trkr);
        continue;
      }
      disarm_timerfd(trkr->time_set.timer_fd);
      protocol_queue.dequeue(trkr);
      tracker_event current_ev = tracker_context.left==0 ? tracker_event::completed: tracker_event::stopped;
      set_announce_url_for_http_tracker(*trkr, current_ev);
      trkr->bool_set.requeueable=false;
      request_t current_request {trkr->net_set.connection, trkr, do_on_success, do_on_failure};
      protocol.http.add_request(current_request);
    }
  }
}

void TrackerManager::update_context(unsigned dwn, unsigned upd) {
  tracker_context.downloaded += dwn;
  tracker_context.uploaded   += upd;
  tracker_context.left       -= dwn;
}
void TrackerManager::announce() {
  while(true) // this should be a boolean flag that shutdowns the tracker manager.
    queue_in_http_requests_auto();
}
void TrackerManager::reannounce() {
  tracker_context.state = trkr_manager_state::reannounce;
}
void TrackerManager::force_reannounce() {
  tracker_context.state = trkr_manager_state::force_reannounce;
}
void TrackerManager::shutdown() {
  tracker_context.state = trkr_manager_state::shutdown;
}
void TrackerManager::set_announce_url_for_udp_trackers() {}
void TrackerManager::queue_in_udp_requests_auto() {}
