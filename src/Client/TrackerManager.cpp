#include "TrackerManager.h"
#include "Constants.h"
#include "HTTPHandler.h"
#include "Hasher.h"
#include <array>
#include <curl/curl.h>
#include <curl/multi.h>
#include <utility>

TrackerManager::TrackerManager(TorrentFile& torrent_, unsigned psp_)
  : torrent(torrent_), listening_port(psp_) {
    /* info hash byte initialization */
    std::string_view info_key = torrent.get_info_key();
    const std::span<const std::byte> hash_byte_view(reinterpret_cast<const std::byte*>(info_key.data()), info_key.size());
    const std::vector<std::byte> info_hash_bytes = Hasher::get_sha1(hash_byte_view);
    info_hash_byte = Hasher::byte_stringify_hash(info_hash_bytes);
    /* info hash byte initialization ends here */
    initiatlize_trackers(torrent.get_tracker_urls());
    partition_initialized_trackers();
}

std::string TrackerManager::get_request_params(unsigned uploaded, unsigned downloaded, unsigned left, unsigned compact, std::string event)
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
    make_param("uploaded", std::to_string(uploaded)),
    make_param("downloaded", std::to_string(downloaded)),
    make_param("left", std::to_string(left)),
    make_param("compact", std::to_string(compact)),
    make_param("event", event)
  };
  auto accumulate_params = [ampersand='&'](std::array<std::string, 8> params) {
    std::string loaded_paramaters;
    for(std::string& param : params) loaded_paramaters += std::move(param) + ampersand;
    loaded_paramaters.pop_back();
    return loaded_paramaters;
  };
  return accumulate_params(params);
}

void TrackerManager::initiatlize_trackers(std::vector<std::string_view> trackers_urls) {
  for(auto url : trackers_urls) {
    Tracker current;
    current.net_d.is_http_not_udp = url.starts_with("http") ? true : false;
    current.net_d.connection = current.net_d.is_http_not_udp ? HTTPHandler::new_easy(current.net_d.data, url) : nullptr;
    tracker_connections.push_back(std::move(current));
  }
  tracker_connections.shrink_to_fit();
}

void TrackerManager::partition_initialized_trackers() {
  for (auto tracker : tracker_connections) {
    if (tracker.net_d.is_http_not_udp)
      http.add_handle(tracker.net_d.connection);
    else //control flow for udp;
      ;
  }
}

TrackerManager::peer_schema TrackerManager::get_peers_http() {
  // invoke http to run get method
  // wait for completion on the first try
  //
  return "";
}
