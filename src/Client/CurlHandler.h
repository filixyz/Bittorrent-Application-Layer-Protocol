#ifndef LIBCURL_HANDLER
#define LIBCURL_HANDLER

#include <cstddef>
#include <curl/curl.h>
#include <string>

struct network_data {
  std::string data;
  std::size_t size;
};

enum LibcurlOptions {
  SET_RES_VAR,
  SET_URL,
  SET_ARGS
};

class CurlHandle {
  CURL *handle;
  static size_t callback_func(const char* data, size_t size, size_t datalen,void *user_data);
public:
  CurlHandle();
  ~CurlHandle();
  unsigned set_option(LibcurlOptions, auto user_field);
  void reset();
  unsigned perform() const;
};

unsigned CurlHandle::set_option(LibcurlOptions option, auto user_field) {
  if (!handle) {
    // come back here later an define a better error code
    return -1;
  }
  CURLcode status;
  switch (option) {
    case SET_RES_VAR: {
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, callback_func);
    status=curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void *)(user_field));
    break;
    }
    case SET_URL: {
    status=curl_easy_setopt(handle, CURLOPT_URL, user_field);
    break;
    }
  }
  return status;
}

#endif
