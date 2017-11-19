#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <chrono>
#include <iostream>
#include <thread>

// http://www.informit.com/articles/article.aspx?p=23618&seqNum=14

void timer_handler(int signum) {
  static int count = 0;
  std::cout << "timer expired " << ++count
            << " timers tid: " << std::this_thread::get_id() << std::endl;
}

void sig_hanadler(int singo) {
  if (singo == SIGINT) {
    std::cout << "recv SIGINT and thread id: " << std::this_thread::get_id()
              << std::endl;
    exit(0);
  }
}

int main() {
  std::cout << "main thread id: " << std::this_thread::get_id() << std::endl;

  struct sigaction sa;
  struct itimerval timer;

  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = &timer_handler;
  sigaction(SIGVTALRM, &sa, NULL);

  timer.it_value.tv_sec = 0;
  timer.it_value.tv_usec = 250000;

  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_usec = 250000;

  setitimer(ITIMER_VIRTUAL, &timer, NULL);

  signal(SIGINT, sig_hanadler);

  while (1) {
  }

  return 0;
}