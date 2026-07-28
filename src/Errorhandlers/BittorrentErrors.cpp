#include "BittorentErrors.h"
#include <iostream>
#include <stdexcept>

bool error_with_reason(const char *reason) {
  std::cerr << reason << '\n';
  return false;
}

Invalid_Bencode_File::Invalid_Bencode_File()
  : std::logic_error("Logic Error: Bencoded File is Invalid") {}

Invalid_Torrent_File::Invalid_Torrent_File()
  : std::logic_error("Logic Error: Bencoded file could not be found") {}

Torrent_File_Not_Found::Torrent_File_Not_Found()
  : std::logic_error("Logic Error: MetaInfo file not found") {}

NO_IP_Support::NO_IP_Support()
  : std::logic_error("Implementation has no IPv4 not IPV6 support") {}

NO_Memory::NO_Memory()
  : std::logic_error("Memory insufficient for alloacation") {}
