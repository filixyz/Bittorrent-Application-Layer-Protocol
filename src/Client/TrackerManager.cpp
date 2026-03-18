#include "TrackerManager.h"
#include "Constants.h"
#include "HTTPHandler.h"
#include "Hasher.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <curl/curl.h>
#include <curl/multi.h>
#include <string>
#include <utility>
#include <queue>

Tracker::Tracker(std::string url_) : url(url_) {};

void TrackerManager::initiatlize_trackers(std::vector<std::string_view> trackers_urls) {
  for(auto url : trackers_urls) {
    Tracker current{std::string(url)};
    current.net_d.is_http_not_udp = url.starts_with("http") ? true : false;
    current.net_d.connection = current.net_d.is_http_not_udp ? HTTPHandler::new_easy(current.net_d.data) : nullptr;
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
    make_param("event", tracker_context.event)
  };
  auto accumulate_params = [ampersand='&'](std::array<std::string, 8> params) {
    std::string loaded_paramaters;
    for(std::string& param : params) loaded_paramaters += std::move(param) + ampersand;
    loaded_paramaters.pop_back();
    return loaded_paramaters;
  };
  return accumulate_params(params);
}

void TrackerManager::set_announce_url_for_http_trackers(tracker_event event) {
  auto set_curl_url = [&, this](Tracker& trkr) {
    if (!trkr.net_d.is_http_not_udp)
      return;
    std::string tracker_id = trkr.tracker_id.empty() ? "" : std::string{'&'}.append("trackerid=").append(trkr.tracker_id);
    std::string request_url = trkr.url + '?' + get_request_params() + tracker_id + event_strings[static_cast<short>(event)];
    curl_easy_setopt(trkr.net_d.connection, CURLOPT_URL, request_url.data());
  };
  std::for_each(tracker_connections.begin(), tracker_connections.end(), set_curl_url);
}

void TrackerManager::queue_in_http_requests(tracker_event event) {
  set_announce_url_for_http_trackers(event);
  //----------LOAD CONST POINTERS TO TRACKERS INTO QUEUE-----------//
  std::queue<const Tracker*> tracker_queue{};
  for (Tracker& t: tracker_connections)
    if(t.net_d.is_http_not_udp)
      tracker_queue.push(&t);
  //----------------------------DONE-------------------------------//
  //   THEN DELAY THIER POPULATION TO CURLM*(http) DEPENDING ON THEIR
  //         INTERVALS FROM THE LAST SUCCESSFUL TRANSACTION        //
  while (!tracker_queue.empty()) {
    const Tracker* current = tracker_queue.front();
    const std::chrono::seconds& interval =
      (event != tracker_event::update && current->time_d.min_interval.count() != -1)
        ? current->time_d.min_interval : current->time_d.interval;
    if (std::chrono::steady_clock::now() - current->time_d.last_transaction_tp >= interval) {
      http.add_handle(current->net_d.connection);
      tracker_queue.pop();
    } else {
      tracker_queue.push(std::move(current));
      tracker_queue.pop();
    }
  }
}

void TrackerManager::set_announce_url_for_udp_trackers() {}
void TrackerManager::queue_in_udp_requests() {}

void TrackerManager::send_requests() {
  //tell libcurl to run requests
  ;
}
void TrackerManager::update_context(){

}
void TrackerManager::announce(tracker_event event) {
  set_announce_url_for_http_trackers(event);
  queue_in_http_requests(event);
  set_announce_url_for_udp_trackers();
  queue_in_udp_requests();
  send_requests();
  parse_responses();
  feed_peer_manager();
}

void TrackerManager::announce() {
  announce(tracker_event::update);
}
