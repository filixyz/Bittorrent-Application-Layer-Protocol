#include "Client.h"

BClient_Instance::BClient_Instance(Torrent_File &file_)
    : file(file_), magnet_url("null"), initialization_already_done(false),
      download_complete(false), seeding(false)

{}

void BClient_Instance::start() const {}
