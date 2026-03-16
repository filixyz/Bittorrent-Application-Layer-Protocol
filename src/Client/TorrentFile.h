#ifndef TORRENT_FILE
#define TORRENT_FILE

#include "../Bencoder/Bencode.h"
#include <filesystem>

class TorrentFile {
private:
  std::map<std::string, Bendata> transcibe;
  const std::map<std::string, Bendata> *info_hash;
  void check_validity_of_transcribe() const;

public:
  TorrentFile(const std::filesystem::path pathname);
  TorrentFile() = delete;
  std::vector<std::string_view> get_tracker_urls() const;
  std::string_view get_info_key() const;
  std::string_view get_torrent_name() const;
  int get_piece_length() const;
  std::string_view get_piece_hash(int index) const;
  bool torrent_is_file() const;
  int get_download_size() const;
};

#endif
