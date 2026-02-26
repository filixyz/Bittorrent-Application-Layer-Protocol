#include "PeerManager.h"
#include <string>
#include <vector>

enum class TrackerProtocol { UDP_MODE, HTTP_MODE };

class TrackerManager {
  std::string tracker_url;
  const TrackerProtocol tracker_mode;
  std::vector<Peer> get_peers_for_http_mode();
  std::vector<Peer> get_peers_for_udp_mode();
public:
  TrackerManager() = default;
  TrackerManager(std::string announce_url);
  void send_request_to_tracker();
  void update_tracker();
};
