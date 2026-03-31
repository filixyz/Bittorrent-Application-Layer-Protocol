#include "HTTPHandler.h"
#include <curl/curl.h>
#include <curl/easy.h>
#include <curl/multi.h>

HTTPHandler::HTTPHandler(int _epoll_fd) {
  handle = curl_multi_init();
  if (!handle)
    ; //handle error
  fds.epoll_fd=_epoll_fd;
  curl_multi_setopt(handle, CURLMOPT_SOCKETFUNCTION, socket_callback);
  curl_multi_setopt(handle, CURLMOPT_SOCKETDATA, &fds);
  curl_multi_setopt(handle, CURLMOPT_TIMERFUNCTION, timer_callback);
  curl_multi_setopt(handle, CURLMOPT_TIMERDATA, &fds);
}
HTTPHandler::~HTTPHandler() { curl_multi_cleanup(handle); }

void HTTPHandler::escape_byte_string(std::string& url) {
  char * escaped_string = curl_escape(url.data(), url.size());
  url.replace(url.begin(), url.end(), escaped_string);
}

CURL* HTTPHandler::new_easy(network_data& user_field) {
  CURL* newE = curl_easy_init();
  curl_easy_setopt(newE, CURLOPT_WRITEFUNCTION, easy_callback);
  curl_easy_setopt(newE, CURLOPT_WRITEDATA, &user_field);
  return newE;
}

void HTTPHandler::add_request(request_t request) {

}
void HTTPHandler::rmv_request(request_t request) {

}

void HTTPHandler::drive() {
  // handle networks events in the supplied
  // epoll fd the driving class/function provided
}

size_t HTTPHandler::easy_callback(const char* data, size_t size, size_t datalen,void *user_data) {
  network_data *mem = (network_data *)(user_data);
  if (!mem) return 0;
  mem->data += data;
  mem->size += datalen;
  return datalen;
}

int HTTPHandler::socket_callback(CURL *easy, curl_socket_t s, int what, void *clientp, void *socketp) {
}

int HTTPHandler::timer_callback(CURLM *multi, long timeout_ms, void *userp) {
}
