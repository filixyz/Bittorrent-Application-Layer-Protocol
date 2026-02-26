#include "LibcurlHandler.h"
#include <curl/curl.h>
#include <curl/easy.h>

size_t callback_func(const char* data, size_t size, size_t datalen,void *user_data) {
  network_data *mem = (network_data *)(user_data);
  if (!mem)
    return 0;
  mem->data += data;
  mem->size += datalen;
  return datalen;
}

LibCurlHandle::LibCurlHandle() { handle = curl_easy_init(); }
LibCurlHandle::~LibCurlHandle() { curl_easy_cleanup(handle); }
unsigned LibCurlHandle::perform() const { return curl_easy_perform(handle); }
