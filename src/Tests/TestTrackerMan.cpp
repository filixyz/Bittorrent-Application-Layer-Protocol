#include "../Client/TrackerManager.h"
#include <chrono>
#include <thread>
int main() {
  TorrentFile torrent {"Torrents/regular-show-complete.torrent"};
  TrackerManager tracker{torrent, 1984};
  std::jthread trkr_service {&TrackerManager::test, &tracker, 200};
  std::this_thread::sleep_for(std::chrono::seconds(10));
  tracker.start();
}
