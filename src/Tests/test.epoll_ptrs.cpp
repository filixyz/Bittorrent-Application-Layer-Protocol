#include <chrono>
#include <cstdint>
#include <ctime>
#include <iostream>
#include <vector>
#include <sys/epoll.h>
#include <sys/time.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <thread>
#include <chrono>
int main() {
  int tf = timerfd_create(CLOCK_REALTIME, TFD_CLOEXEC);

  std::cout << tf << " Thsis is the timer fd"<<'\n';
  struct timespec clock_now;
  clock_gettime(CLOCK_REALTIME, &clock_now);
  clock_now.tv_sec+=175;

  struct itimerspec timer {{0, 0}, clock_now };
  int fd = epoll_create1(EPOLL_CLOEXEC);
  struct epoll_event event;
  std::cout << &event << " This is the address of the initial event\n";
  event.events=EPOLLIN;  event.data.fd=tf;
  auto addr = &event.data.fd;
  int i = epoll_ctl(fd, EPOLL_CTL_ADD, tf, &event);
  std::chrono::time_point<std::chrono::steady_clock> stamp = std::chrono::steady_clock::now();
  timerfd_settime(tf, TFD_TIMER_ABSTIME, &timer, nullptr);
  *addr = 6;
  std::cout << event.data.fd << " changed fd" << '\n' ;
  int somethin = 557;
  std::cout << "This is address of somethin" << &somethin << '\n';

  epoll_event events_v[5];
  int size=0;
  //char = 0x2c:
  std::cout << sizeof(void*) << "size\n";
  std::cout << sizeof(unsigned long) << "size\n";
  unsigned long pad = 0x2c;
  std::cout << &events_v[0] << "THis is the address of where the epoll_event is stored\n";
  std::cout << &event << "This is the address of the actual event\n";
  int count=0;
  //std::this_thread::sleep_for(std::chrono::seconds(13));
  while((size=epoll_wait(fd, events_v , 5, -1))!=0) {
    std::cout << "returned events " << size << '\n';

    for(int i=0; i<size; ++i) {
      if (events_v[i].events & EPOLLIN)
        std::cout << "This is an input event\n";
      std::chrono::seconds cl = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - stamp);
      std::cout << cl.count() << " seconds have passed \n";
      uint64_t rad{};
      read(tf, &rad, sizeof(uint64_t));
      std::cout << (int)rad << " i am what you read from tf fd\n";
      std::cout << &events_v[i] << '\n';
      std::cout << events_v[i].data.fd << "This is the fd" << '\n';
      std::cout << &events_v[i].data.fd  << "...." << (addr);
      std::cout << "\n\n";
      std::cout << ++count << " This is the count\n";
    }
  }
  std::cout << event.data.fd << " changed fd" << '\n' ;
}
