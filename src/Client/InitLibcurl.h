#ifndef CURL_INIT
#define CURL_INIT
#include <curl/curl.h>

constexpr long CURL_FLAG=CURL_GLOBAL_ALL;

class InitLibcurl{
  CURLcode success;
public:
  InitLibcurl();
  ~InitLibcurl();
  explicit operator bool() const;
};

#endif
