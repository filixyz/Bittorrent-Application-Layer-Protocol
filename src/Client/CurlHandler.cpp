#include "CurlHandler.h"
#include <curl/curl.h>
#include <curl/easy.h>

size_t CurlHandle::callback_func(const char* data, size_t size, size_t datalen,void *user_data) {
  network_data *mem = (network_data *)(user_data);
  if (!mem)
    return 0;
  mem->data += data;
  mem->size += datalen;
  return datalen;
}

void CurlHandle::escape_byte_string(std::string& url) {
  char * escaped_string = curl_escape(url.data(), url.size());
  url.replace(url.begin(), url.end(), escaped_string);
}

CurlHandle::CurlHandle() { handle = curl_easy_init(); }
CurlHandle::~CurlHandle() { curl_easy_cleanup(handle); }
unsigned CurlHandle::perform() const { return curl_easy_perform(handle); }
