#pragma once

#include "poller.h"
#include <sys/epoll.h>
#include <vector>

class Epoller : public Poller {
public:
	Epoller();
	~Epoller();

	bool AddSocket(int sock, int events, void *userPtr);
	bool ModSocket(int sock, int events, void *userPtr);
	bool DelSocket(int sock, int events);

	int Poll(std::vector<FiredEvent> &events, std::size_t maxEvent, int timeoutMs);

private:
	std::vector<epoll_event> events_;
};
