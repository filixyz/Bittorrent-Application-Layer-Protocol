#ifndef TORRENT_FILE
#define TORRENT_FILE

#include "../Bencoder/Bencode.h"
#include <filesystem>

class Torrent_File {
private:
  std::map<std::string, Bendata> transcibe;
  std::map<std::string, Bendata> *info_hash;
  void check_validity_of_transcribe() const;

public:
  Torrent_File(const std::filesystem::path pathname);
  Torrent_File() = delete;
  std::string get_tracker_url() const;
  std::string get_info_key() const;
  std::string get_torrent_name() const;
  int get_piece_length() const;
  std::string get_piece_hash(int index) const;
  bool torrent_is_file() const;
  int get_download_size() const;
};

#endif
