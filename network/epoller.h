// Copyright 2022 The uhp-sql Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

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

  int Poll(std::vector<FiredEvent> &events, std::size_t maxEvent,
           int timeoutMs);

 private:
  std::vector<epoll_event> events_;
};
