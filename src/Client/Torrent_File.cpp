#include "Torrent_File.h"
#include "../Errorhandlers/BittorentErrors.h"
#include <fstream>

constexpr int HASH_STRING_LENGTH = 20;

Torrent_File::Torrent_File(const std::filesystem::path pathname) {
  std::ifstream torrent_file{pathname};
  if (!torrent_file)
    throw Torrent_File_Not_Found{};
  try {
    Bendata parsed_bendata(bendecode_from_file(torrent_file));
    transcibe = std::move(parsed_bendata.get_data<ben::dic>());
    check_validity_of_transcribe();
    info_hash = &(transcibe.find("info")->second.get_data<ben::dic>());
  } catch (...) {
    throw Invalid_Torrent_File{};
  }
}

void Torrent_File::check_validity_of_transcribe() const {
  // implement later
}

std::string Torrent_File::get_tracker_url() const {
  return transcibe.find("announce")->second.get_data<std::string>();
}
std::string Torrent_File::get_info_key() const {
  return transcibe.find("info")->second.get_encode();
}
std::string Torrent_File::get_torrent_name() const {
  return info_hash->find("name")->second.get_data<std::string>();
}
int Torrent_File::get_piece_length() const {
  return info_hash->find("piece length")->second.get_data<int>();
}

std::string Torrent_File::get_piece_hash(int index) const {
  const std::string &pieces_hash =
      info_hash->find("pieces")->second.get_data<std::string>();
  int hash_index = index * HASH_STRING_LENGTH;
  return pieces_hash.substr(hash_index, HASH_STRING_LENGTH);
}

bool Torrent_File::torrent_is_file() const {
  auto end_itr = info_hash->end();
  auto file_itr = info_hash->find("length");
  return file_itr != end_itr ? true : false;
}

int Torrent_File::get_download_size() const {
  return torrent_is_file() ? info_hash->find("length")->second.get_data<int>()
                           : 0;
}
