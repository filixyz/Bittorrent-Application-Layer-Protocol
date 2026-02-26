#include "Client/InitLibcurl.h"
#include "Client/LibcurlHandler.h"
#include <curl/curl.h>
#include <iostream>
#include <cassert>

int main(){
  InitLibcurl global_curl_initialized{};
  assert(global_curl_initialized);
  LibCurlHandle handle{};
  network_data result{};
  handle.set_option(SET_URL, "https://google.com");
  handle.set_option(SET_RES_VAR, &result);
  handle.perform();
  std::cout << result.data << '\n' << result.size << '\n';
}
