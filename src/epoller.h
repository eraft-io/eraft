/**
 * @file epoller.h
 * @author ERaftGroup
 * @brief
 * @version 0.1
 * @date 2023-06-17
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <sys/epoll.h>

#include <vector>

#include "poller.h"

class Epoller : public Poller {
 public:
  Epoller();
  ~Epoller();

  bool AddSocket(int sock, int events, void *userPtr);
  bool ModSocket(int sock, int events, void *userPtr);
  bool DelSocket(int sock, int events);

  int Poll(std::vector<FiredEvent> &events,
           std::size_t              maxEvent,
           int                      timeoutMs);

 private:
  std::vector<epoll_event> events_;
};
