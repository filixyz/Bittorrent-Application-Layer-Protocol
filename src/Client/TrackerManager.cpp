#include "TrackerManager.h"
#include "Constants.h"
#include "CurlHandler.h"
#include "Hasher.h"
#include <array>
#include <curl/curl.h>
#include <string>
#include <utility>

TrackerManager::TrackerManager(TorrentFile& torrent_, unsigned psp_)
  : tracker(), torrent(torrent_), listening_port(psp_) {
    std::string_view info_key = torrent.get_info_key();
    const std::span<const std::byte> hash_byte_view(reinterpret_cast<const std::byte*>(info_key.data()), info_key.size());
    const std::vector<std::byte> info_hash_bytes = Hasher::get_sha1(hash_byte_view);
    info_hash_byte = Hasher::byte_stringify_hash(info_hash_bytes);
}

std::string TrackerManager::get_request_params(unsigned uploaded, unsigned downloaded, unsigned left, unsigned compact, std::string event)
{
  auto make_param = [](std::string_view key, std::string_view value) {
    return std::string(key).append(1, '=').append(value);
  };
  std::string info_hash_byte_escaped_copy = info_hash_byte;
  CurlHandle::escape_byte_string(info_hash_byte_escaped_copy);
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
    return loaded_paramaters;
  };
  return accumulate_params(params);
}

TrackerManager::peer_schema TrackerManager::get_peers_http() {
  std::string tracker_url{"http://tracker.opentrackr.org:1337/announce"};
  std::string url_parameters = get_request_params(0, 0, torrent.get_download_size(), 1, "started");
  std::string request_url = std::move(tracker_url) + '?' + std::move(url_parameters);
  std::cout << request_url;
  tracker.set_option(SET_URL, request_url.data());
  network_data response;
  tracker.set_option(SET_RES_VAR, &response);
  tracker.perform();
  return response.data;
}
