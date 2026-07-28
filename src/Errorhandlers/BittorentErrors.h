#ifndef BITTORRENT_ERRORS
#define BITTORRENT_ERRORS

#include <stdexcept>

bool error_with_reason(const char *reason);

struct Torrent_File_Not_Found : public std::logic_error {
  Torrent_File_Not_Found();
};

struct Invalid_Bencode_File : public std::logic_error {
  Invalid_Bencode_File();
};

struct Invalid_Torrent_File : public std::logic_error {
  Invalid_Torrent_File();
};

struct NO_IP_Support : public std::logic_error {
  NO_IP_Support();
};

struct NO_Memory : public std::logic_error {
  NO_Memory();
};

#endif
