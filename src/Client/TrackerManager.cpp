#include "TrackerManager.h"
#include "Constants.h"
#include "CurlHandler.h"
#include "Hasher.h"

TrackerManager::TrackerManager(TorrentFile& torrent_, unsigned psp_)
  : tracker(), torrent(torrent_), listening_port(psp_) {
}

TrackerManager::peer_schema TrackerManager::get_peers_http() {
  std::string tracker_url = torrent.get_tracker_url();
  auto make_param = [](std::string key, std::string value) { return key + '=' + value; };
  std::vector<std::string> params;
  params.push_back(make_param("info_hash", /** provide hash here **/));
  params.push_back(make_param("peer_id", client::constants::client_id ));
  params.push_back(make_param("port", std::to_string(listening_port)));
  params.push_back(make_param("uploaded", "0"));
  params.push_back(make_param("downloaded", "0"));
  params.push_back(make_param("left", std::to_string(torrent.get_download_size())));
  params.push_back(make_param("compact", "1"));
  params.push_back(make_param("event", "started"));
  auto accumulate_params = [](std::vector<std::string> params) {
    std::string loaded_paramaters;
    for(std::string& param:params) loaded_paramaters += (param + "&");
    return loaded_paramaters;
  };
  std::string request_url = std::move(tracker_url) +'?' + accumulate_params(params);
  CurlHandle::escape_url(request_url);
  tracker.set_option(SET_URL, request_url);
  std::string response;
  tracker.set_option(SET_RES_VAR, response);
  tracker.perform();
  return response;
}
