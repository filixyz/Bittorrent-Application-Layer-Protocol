#include "InitCurl.h"
#include <cstring>
#include <curl/curl.h>

InitCurl::InitCurl(){
  success=curl_global_init(CURL_FLAG);
}
InitCurl::~InitCurl(){
  curl_global_cleanup();
}
InitCurl::operator bool() const {
  return !success;
}
