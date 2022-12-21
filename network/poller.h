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

#include <vector>

enum EventType {
  EventTypeRead = 0x1,
  EventTypeWrite = 0x1 << 1,
  EventTypeError = 0x1 << 2,
};

struct FiredEvent {
  int events;
  void* userdata;

  FiredEvent() : events(0), userdata(0) {}
};

class Poller {
 public:
  Poller() : multiplexer_(-1) {}

  virtual ~Poller() {}

  virtual bool AddSocket(int sock, int events, void* userPtr) = 0;
  virtual bool ModSocket(int sock, int events, void* userPtr) = 0;
  virtual bool DelSocket(int sock, int events) = 0;

  virtual int Poll(std::vector<FiredEvent>& events, std::size_t maxEv,
                   int timeoutMs) = 0;

 protected:
  int multiplexer_;
};
