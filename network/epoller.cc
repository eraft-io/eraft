#include "epoller.h"

#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

namespace Epoll
{
bool ModSocket(int epfd, int socket, uint32_t events, void *ptr);

bool AddSocket(int epfd, int socket, uint32_t events, void *ptr)
{
	if (socket < 0)
		return false;

	epoll_event ev;
	ev.data.ptr = ptr;

	ev.events = 0;
	if (events & EventTypeRead)
		ev.events |= EPOLLIN;
	if (events & EventTypeWrite)
		ev.events |= EPOLLOUT;

	return 0 == epoll_ctl(epfd, EPOLL_CTL_ADD, socket, &ev);
}

bool DelSocket(int epfd, int socket)
{
	if (socket < 0)
		return false;

	epoll_event dummy;

	return 0 == epoll_ctl(epfd, EPOLL_CTL_DEL, socket, &dummy);
}

bool ModSocket(int epfd, int socket, uint32_t events, void *ptr)
{
	if (socket < 0)
		return false;

	epoll_event ev;
	ev.data.ptr = ptr;

	ev.events = 0;
	if (events & EventTypeRead)
		ev.events |= EPOLLIN;
	if (events & EventTypeWrite)
		ev.events |= EPOLLOUT;

	return 0 == epoll_ctl(epfd, EPOLL_CTL_MOD, socket, &ev);
}
}

Epoller::Epoller()
{
	multiplexer_ = ::epoll_create(512);
}

Epoller::~Epoller()
{
	if (multiplexer_ != -1)
		::close(multiplexer_);
}

bool Epoller::AddSocket(int sock, int events, void *userPtr)
{
	if (Epoll::AddSocket(multiplexer_, sock, events, userPtr))
		return true;

	return (errno == EEXIST) && ModSocket(sock, events, userPtr);
}

bool Epoller::DelSocket(int sock, int events)
{
	return Epoll::DelSocket(multiplexer_, sock);
}

bool Epoller::ModSocket(int sock, int events, void *userPtr)
{
	if (events == 0)
		return DelSocket(sock, 0);

	if (Epoll::ModSocket(multiplexer_, sock, events, userPtr))
		return true;

	return errno == ENOENT && AddSocket(sock, events, userPtr);
}

int Epoller::Poll(std::vector<FiredEvent> &events, size_t maxEvent, int timeoutMs)
{
	if (maxEvent == 0)
		return 0;

	while (events_.size() < maxEvent) {
		events_.resize(2 * events_.size() + 1);
	}

	int nFired = TEMP_FAILURE_RETRY(
		::epoll_wait(multiplexer_, &events_[0], maxEvent, timeoutMs));
	if (nFired == -1 && errno != EINTR && errno != EWOULDBLOCK)
		return -1;

	if (nFired > 0 && static_cast<size_t>(nFired) > events.size())
		events.resize(nFired);

	for (int i = 0; i < nFired; ++i) {
		FiredEvent &fired = events[i];
		fired.events = 0;
		fired.userdata = events_[i].data.ptr;

		if (events_[i].events & EPOLLIN)
			fired.events |= EventTypeRead;
		if (events_[i].events & EPOLLOUT)
			fired.events |= EventTypeWrite;
		if (events_[i].events & (EPOLLERR | EPOLLHUP))
			fired.events != EventTypeError;
	}

	return nFired;
}
