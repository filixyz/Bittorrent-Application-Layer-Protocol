#include "TrackerManager.h"
#include "../Bencoder/Bencode.h"
#include <sstream>
#include <string>

/**
 * A Tracker cannot be default initialized nor moved
 * It can be initialized with a url though and it can also be
 * copy constructed, when copy constructed all data members will
 * be identical (in value) with the Tracker object copied from
 * but the ev::timer members are not copied. These limitiations
 * are due to ev::timer having no public copy/move constructors set. It also makes
 * sense logically if you think about it, so it's more of a logical limitation
 * than a design one.
 */

TrackerManager::Tracker::Tracker(std::string url) : HTTPRequest(), nest{} {
  nest.announce_urls.push_back(std::move(url));
  nest.current_announce_url_index=0;
};

TrackerManager::Tracker::Tracker(const Tracker& other) {
  // A copied Traker is assumed to be used in another tracker manager
  // So copying the time watchers, failure count and queue_index members
  // is illogical.
  nest.bool_set = other.nest.bool_set;
  nest.time_set.min_interval = other.nest.time_set.min_interval;
  nest.time_set.interval = other.nest.time_set.interval;
  nest.tracker_id = other.nest.tracker_id;
}

void TrackerManager::Tracker::do_on_success() {
  std::cout << "tracker succeeded\n";
  nest.failure_count=0;
  nest.failed_url_index=-1;
  nest.bool_set.active_flag=true;
  // nest.bool_set.update_state=true;  // !!!bug!!! what if successful at backend level (http-200ok/udp-whatevs) but contains "failure reason" key?

  // place bendecode parse and tracker timer updates here
  // send peers to peermanager here

  std::cout << user_space.data << '\n';
  if (!user_space.data.empty()) {
    Bendata parse;
    std::istringstream bencode(user_space.data);
    get_bendata_from_stream(bencode, parse);
    ben::dic& parsed_dict = parse.get_data<ben::dic>();
    // ----------> EXCEPTION MAY HAPPEN HERE IF BENCODE IS ERRORNEOUS.
    if (parsed_dict.contains("failure reason"))
      std::cout << parsed_dict["failure reason"];
    else
      nest.bool_set.update_state=true;
    if (parsed_dict.contains("warning message"))
      std::cout << parsed_dict["warning message"];
    if (parsed_dict.contains("interval"))
      nest.time_set.interval = parsed_dict["interval"].get_data<ben::num>();
    if (parsed_dict.contains("min interval"))
      nest.time_set.min_interval = parsed_dict["min interval"].get_data<ben::num>();
    else
      nest.time_set.min_interval = 0;
    if (parsed_dict.contains("tracker id"))
      nest.tracker_id = parsed_dict["tracker id"].get_data<ben::str>();
    if (parsed_dict.contains("peers"))
      std::cout << parsed_dict["peers"];
  }

  if(nest.time_set.min_interval==0 || nest.time_set.min_interval==-1)
    nest.bool_set.only_one_timer=true;

  if(!nest.bool_set.only_one_timer) {
    arm_timer(nest.time_set.min_timer_w, nest.time_set.min_interval);
    nest.bool_set.interruptible=false;
  }
  arm_timer(nest.time_set.timer_w, nest.time_set.interval);

  if (nest.bool_set.requeueable)
    return_to_manager_space();
}

void TrackerManager::Tracker::do_on_failure() {
  std::cout << "tracker failed\n";
  nest.bool_set.only_one_timer=true;
  int retry_for_new_url = 5;//seconds
  if (nest.failed_url_index==-1)
    nest.failed_url_index=nest.current_announce_url_index;
  // failsafe for unimplemeneted udp protocol mechanism
  // once implemented remove `seek_to_next_url()` from
  // while loop; it should run only once.
  while (http_mode==false) seek_to_next_url();                    // UDP FAILSAFE: BUG!: Might seek forever if tracker has no http url (false alarm)
  nest.time_set.interval = retry_for_new_url;
  if (nest.current_announce_url_index==nest.failed_url_index) {
    nest.failure_count++;
    nest.bool_set.active_flag=false;
    nest.time_set.interval = get_retry_seconds(this);
  }
  arm_timer(nest.time_set.timer_w, nest.time_set.interval);
  if (nest.bool_set.requeueable)
    return_to_manager_space();
}

void TrackerManager::Tracker::return_to_manager_space() {
  *nest.queue_ptr = this;
  ++(*nest.trkrs_in_trkrspace_ref);
}

void TrackerManager::Tracker::send_to_protocol_space() {
  *nest.queue_ptr = nullptr;
  --(*nest.trkrs_in_trkrspace_ref);
}

std::string TrackerManager::Tracker::get_url() {
  return nest.announce_urls[nest.current_announce_url_index];
}

void TrackerManager::Tracker::seek_to_next_url() {
  auto& index = ++nest.current_announce_url_index;
  auto& urls  = nest.announce_urls;
  if ( index >= urls.size() ) index=0;  // possible bug: int and size_t byte count mismatch :: FIXED
  http_mode = urls[index].starts_with("http") ? true : false;
}
