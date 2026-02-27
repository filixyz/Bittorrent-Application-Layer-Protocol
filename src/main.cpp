#include "Client/InitCurl.h"
#include "Client/CurlHandler.h"
#include <curl/curl.h>
#include <iostream>
#include <cassert>

int main(){
  InitCurl global_curl_initialized{};
  assert(global_curl_initialized);
  CurlHandle handle{};
  network_data result{};
  handle.set_option(SET_URL, "https://google.com");
  handle.set_option(SET_RES_VAR, &result);
  handle.perform();
  std::cout << result.data << '\n' << result.size << '\n';
}
