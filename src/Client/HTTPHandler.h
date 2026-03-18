#ifndef LIBCURL_HANDLER
#define LIBCURL_HANDLER

#include <cstddef>
#include <curl/curl.h>
#include <string>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <system_error>
#include "Constants.h"

struct network_data {
  std::string data;
  std::size_t size;
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
  void add_handle(CURL*);
  unsigned rmv_handle(CURL*);
  unsigned get() const;
  unsigned post() const;
  void reset();
};

#endif
