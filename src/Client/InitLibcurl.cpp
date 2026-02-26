#include "InitLibcurl.h"
#include <cstring>
#include <curl/curl.h>

InitLibcurl::InitLibcurl(){
  success=curl_global_init(CURL_FLAG);
}
InitLibcurl::~InitLibcurl(){
  curl_global_cleanup();
}
InitLibcurl::operator bool() const {
  return !success;
}
