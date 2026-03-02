#include "TrackerManager.h"
#include <crypto++/sha.h>
#include "Constants.h"



TrackerManager::TrackerManager(TorrentFile& torrent_, unsigned psp_)
  : tracker(), torrent(torrent_), peer_socket_port(psp_) {
}


std::vector<TrackerManager::peer_schema>TrackerManager::get_peers_http() {
  std::string tracker_url = torrent.get_tracker_url();
  // exception can throw here if std::string size exceeds max_size
  // This can happen is inforsafe guard later
  auto accumulate_params = [](std::vector<std::string> params){ return  };
  std::string paramaters = accumulate_params({});
}
