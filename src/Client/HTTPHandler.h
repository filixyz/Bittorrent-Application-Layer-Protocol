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
#include <ev++.h>

class HTTPHandler {
  struct network_data {
    std::string data; std::size_t size;
  };

  CURLM *handle;
  ev::dynamic_loop& event_loop;
  ev::timer curl_timer;

  static size_t easy_callback(const char* data, size_t size, size_t datalen,void *user_data);
  static int socket_callback(CURL *easy, curl_socket_t s, int what, void *clientp, void *socketp);
  static int timer_callback(CURLM *multi, long timeout_ms, void *userp);
  static CURL* new_easy(network_data&);

public:
  class HTTPRequest;
  static void escape_byte_string(std::string&);
  HTTPHandler(ev::dynamic_loop&);
  ~HTTPHandler();
  void add_request(HTTPRequest*);
  void rmv_request(HTTPRequest*);
  void drive_sockt(ev::io&, int);
  void drive_timer(ev::timer&, int);
  void start_protocol();
  void reset();
};

class HTTPHandler::HTTPRequest {
  virtual void do_on_success()=0;
  virtual void do_on_failure()=0;
public:
  CURL* connection;   network_data user_space;
  HTTPRequest();      virtual ~HTTPRequest();
  friend HTTPHandler;
};

using HTTPRequest = class HTTPHandler::HTTPRequest;

#endif
