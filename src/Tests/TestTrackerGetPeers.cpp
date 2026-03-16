#include "../Client/TrackerManager.h"
#include "../Client/Hasher.h"
int main() {
  TorrentFile torrent ("Torrents/redacted");
  TrackerManager tracker (torrent, 6881);

  //std::string compact_response = tracker.get_peers_http();
  //std::cout << compact_response << '\n' << '\n';
  const std::span<const std::byte> byte_view (reinterpret_cast<const std::byte*>(torrent.get_info_key().data()), torrent.get_info_key().length());
  std::cout << Hasher::hex_stringify_hash(Hasher::get_sha1(byte_view)) << '\n';
  for(auto& i : tracker.get_trackers())
    std::cout << i << '\n';
}
