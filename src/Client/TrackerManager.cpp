#include "TrackerManager.h"
#include "Constants.h"
#include "Hasher.h"
#include <array>
#include <cstdlib>
#include <ctime>
#include <curl/curl.h>
#include <curl/multi.h>
#include <string>
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
  // if enqueu flag is true, enqueue back
}
void do_on_failure(void * tracker) {
  // set tracker.active to false
  // if enqueue flag is true, enqueue back
}

void TrackerManager::queue_in_http_requests_auto() {
  for (auto* trkr : protocol_queue.http) {
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
  // Value of epoll here should be the number of trackers in http mode * 2;
  constexpr int SOME_VALUE=100;
  std::array<struct epoll_event, SOME_VALUE> events;
  int n = epoll_wait(epoll_fd, events.data(), SOME_VALUE, -1);
  for(int index=0; index<n; ++index) {
    Tracker* trkr = reinterpret_cast<Tracker*>(events[index].data.ptr);
    if(trkr->bool_set.only_one_timer) {
      if(tracker_context.event==tracker_event::normal || tracker_context.event==tracker_event::update) {
        //----------NON INTERRUPT MODE HANDLER-----------//
        // modify curl handle for a start request if update flag is not set else modify
        // for update request with current context send to protocol to handle
        // change protocol_queue_index to nullptr
        // announce and reannoounce is practically the same thing for trackers that
        // abide to one timer.
        protocol_queue.dequeue(trkr);
        if(!trkr->bool_set.update_flag)
          set_announce_url_for_http_tracker(*trkr, tracker_event::started);
        else
          set_announce_url_for_http_tracker(*trkr, tracker_event::update);
        request_t current_request {trkr->net_set.connection, trkr, do_on_success, do_on_failure};
        protocol.http.add_request(current_request);

        //------NON INTERRUPT MODE HANDLER ENDS HERE------//
      } else {
        //------------INTERRUPT MODE HANDLER -------------//
        if(!trkr->bool_set.update_flag && !trkr->bool_set.active_flag) {
          // Tracker has never been active; not obligated to inform it
          // set index to nullptr
          // don't queue into protocol
          trkr->bool_set.requeueable=false;
          protocol_queue.dequeue(trkr);
        }
        if(trkr->bool_set.update_flag) {
          // tracker was once active or is active, obligated to try informing it we are out of the swarm
          // set index to nullptr set curl url to reflect current context
          // set do not reinsert flag queue into protocol
          protocol_queue.dequeue(trkr);
          set_announce_url_for_http_tracker(*trkr, tracker_context.event);
          trkr->bool_set.requeueable=false;
          request_t current_request {trkr->net_set.connection, trkr, do_on_success, do_on_failure};
          protocol.http.add_request(current_request);
        }
        //--------INTERRUPT MODE HANDLER ENDS HERE--------//
      }
    }
    if (trkr->bool_set.only_one_timer==false) {
      if(tracker_context.event==tracker_event::normal || tracker_context.event==tracker_event::update) {
        if(trkr->bool_set.interruptible == false) { // this is a min_interval so set interrupt flag
          trkr->bool_set.interruptible=true;
          continue;
        }
        // place code for handling non interrupt mode here
        protocol_queue.dequeue(trkr);
        if(!trkr->bool_set.update_flag)
          set_announce_url_for_http_tracker(*trkr, tracker_event::started);
        else
          set_announce_url_for_http_tracker(*trkr, tracker_event::update);
        request_t current_request {trkr->net_set.connection, trkr, do_on_success, do_on_failure};
        protocol.http.add_request(current_request);

      } else {
        if(trkr->bool_set.interruptible==false) { // this is a min_interval; set interrupt flag; disarm normal timer
          trkr->bool_set.interruptible=true;
          struct itimerspec disarm {{0, 0}, {0, 0}};
          while(timerfd_settime(trkr->time_set.timer_fd, TFD_TIMER_ABSTIME, &disarm, nullptr)==-1);
        }
        // place code for handling interrupt mode here
        if(!trkr->bool_set.update_flag && !trkr->bool_set.active_flag) { // might logically never run.
          trkr->bool_set.requeueable=false;
          protocol_queue.dequeue(trkr);
        }
        if(trkr->bool_set.update_flag) {
          protocol_queue.dequeue(trkr);
          set_announce_url_for_http_tracker(*trkr, tracker_context.event);
          trkr->bool_set.requeueable=false;
          request_t current_request {trkr->net_set.connection, trkr, do_on_success, do_on_failure};
          protocol.http.add_request(current_request);
        }
      }
    }
  }
  // once we out of the loop it should mean that we've handled
  // a snapshot of trackers that was sent to us either in normal state
  // update state (reannouncement) or end state (completed or stopping)
  // but the only recoverable state in all this is the update state.
  if(tracker_context.event == tracker_event::update) {
    for(Tracker* trkr : protocol_queue.http) {
      if (trkr != nullptr && !trkr->bool_set.only_one_timer && trkr->bool_set.interruptible) {
        // since we are doing what the normal interval would had done we should disarm the main
        // interval timer. so epoll doesnt wake on it
        struct itimerspec disarm {{0, 0}, {0, 0}};
        while(timerfd_settime(trkr->time_set.timer_fd, TFD_TIMER_ABSTIME, &disarm, nullptr)==-1);
        // then handle non interrupt reannounce
        protocol_queue.dequeue(trkr);
        if(!trkr->bool_set.update_flag)
          set_announce_url_for_http_tracker(*trkr, tracker_event::started);
        else
          set_announce_url_for_http_tracker(*trkr, tracker_event::update);
        request_t current_request {trkr->net_set.connection, trkr, do_on_success, do_on_failure};
        protocol.http.add_request(current_request);
      }
    }
    tracker_context.event = tracker_event::normal;
  }

  if(tracker_context.event==tracker_event::completed || tracker_context.event==tracker_event::stopped) {
    for(Tracker* trkr : protocol_queue.http) {
      // the below condition satisfies when a trackers
      // min interval timer has expired in a non interrupt state
      // (but it's normal timer is till active) so epoll couldn't catch it
      if (trkr != nullptr && !trkr->bool_set.only_one_timer && trkr->bool_set.interruptible) {
        // disarm normal interval
        struct itimerspec disarm {{0, 0}, {0, 0}};
        while(timerfd_settime(trkr->time_set.timer_fd, TFD_TIMER_ABSTIME, &disarm, nullptr)==-1);
        // then handle then handle interrupt
        if(!trkr->bool_set.update_flag && !trkr->bool_set.active_flag) { // might logically never run.
          trkr->bool_set.requeueable=false;
          protocol_queue.dequeue(trkr);
        }
        if(trkr->bool_set.update_flag) {
          protocol_queue.dequeue(trkr);
          set_announce_url_for_http_tracker(*trkr, tracker_context.event);
          trkr->bool_set.requeueable=false;
          request_t current_request {trkr->net_set.connection, trkr, do_on_success, do_on_failure};
          protocol.http.add_request(current_request);
        }
      }
    }
  }
}
void TrackerManager::update_context(unsigned dwn, unsigned upd) {
  tracker_context.downloaded += dwn;
  tracker_context.uploaded   += upd;
  tracker_context.left       -= dwn;
}
void TrackerManager::announce() {
  queue_in_http_requests_auto();
}
void TrackerManager::reannounce() {
  tracker_context.event = tracker_event::update;
}
void TrackerManager::set_announce_url_for_udp_trackers() {}
void TrackerManager::queue_in_udp_requests_auto() {}
