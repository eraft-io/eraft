// MIT License

// Copyright (c) 2022 eraft dev group

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef ERAFT_POLLER_H_
#define ERAFT_POLLER_H_

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

#endif