#ifndef LIBCURL_HANDLER
#define LIBCURL_HANDLER

#include <cstddef>
#include <curl/curl.h>
#include <string>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <system_error>
#include "Constants.h"
#include <functional>
//#include "TrackerManager.h

struct network_data {
  std::string data;
  std::size_t size;
};

struct request_t {
  CURL* easy_h;
  void* user_ptr;
  std::function<void(void*)> do_on_success;
  std::function<void(void*)> do_on_failure;
};

class HTTPHandler {
  CURLM *handle;
  struct file_descriptors {
    int epoll_fd;
    int timer_fd;
  };
  file_descriptors fds;
  static size_t easy_callback(const char* data, size_t size, size_t datalen,void *user_data);
  static int socket_callback(CURL *easy, curl_socket_t s, int what, void *clientp, void *socketp);
  static int timer_callback(CURLM *multi, long timeout_ms, void *userp);

public:
  HTTPHandler();
  ~HTTPHandler();
  static CURL* new_easy(network_data&);
  static void escape_byte_string(std::string&);
  void add_request(request_t);
  void rmv_request(request_t);
  void reset();
};

#endif
