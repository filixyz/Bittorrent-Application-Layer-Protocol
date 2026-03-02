#include "../Client/TorrentFile.h"
#include <ios>

int main() {
  TorrentFile tfile{
      "Torrents/F2E7E319FC4CF6868456BF384E04E0B33B3AC4FC.torrent"};
  std::cout << std::boolalpha;
  std::cout << tfile.get_info_key() << '\n';
  std::cout << "Torrent Name:\t\t" << tfile.get_torrent_name() << '\n';
  std::cout << "Torrent Size:\t\t" << tfile.get_download_size() << '\n';
  std::cout << "Torrent Tracker:\t" << tfile.get_tracker_url() << '\n';
  std::cout << "Torrent Piece length:\t" << tfile.get_piece_length() << '\n';
  std::cout << "Torrent Piece Hash[1]:\t" << tfile.get_piece_hash(0) << '\n';
  std::cout << "Torrent File?:\t\t" << tfile.torrent_is_file() << '\n';
}
