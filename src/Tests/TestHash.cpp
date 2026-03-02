#include "../Client/Hasher.h"
#include "../Client/TorrentFile.h"
#include <cstddef>

int main() {
  TorrentFile torrent {"Torrents/9B6E27C16DADE8377225389B7FC57130A50B847D.torrent"};
  std::string info_key_s = torrent.get_info_key();
  // byte view
  const std::span<std::byte> bytes(reinterpret_cast<std::byte*>(info_key_s.data()), info_key_s.size());
  std::vector<std::byte> hash_digest = Hasher::get_sha1(bytes);
  std::cout << Hasher::hex_stringify_hash(hash_digest) << '\n';
}
