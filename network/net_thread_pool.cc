#include "net_thread_pool.h"
#include "epoller.h"
#include "stream_socket.h"
#include <cassert>
#include <errno.h>
#include <mutex>

namespace Internal
{

NetThread::NetThread() : runnning_(true), newCnt_(0)
{
	poller_.reset(new Epoller);
}

NetThread::~NetThread()
{
}

void NetThread::AddSocket(PSOCKET task, uint32_t events)
{
	std::lock_guard<std::mutex> guard(mutex_);

	newTasks_.push_back(std::make_pair(task, events));
	++newCnt_;

	assert(newCnt_ == static_cast<int>(newTasks_.size()));
}

void NetThread::ModSocket(PSOCKET task, uint32_t events)
{
	poller_->ModSocket(task->GetSocket(), events, task.get());
}

void NetThread::RemoveSocket(PSOCKET task, uint32_t events)
{
	poller_->DelSocket(task->GetSocket(), events);
}

void NetThread::_TryAddNewTasks()
{
	if (newCnt_ > 0 && mutex_.try_lock()) {
		NewTasks tmp;
		newTasks_.swap(tmp);
		newCnt_ = 0;
		mutex_.unlock();

		auto iter(tmp.begin()), end(tmp.end());

		for (; iter != end; ++iter)
			_AddSocket(iter->first, iter->second);
	}
}

void NetThread::_AddSocket(PSOCKET task, uint32_t events)
{
	// add to epoll pool
	if (poller_->AddSocket(task->GetSocket(), events, task.get()))
		tasks_.push_back(task);
}

void RecvThread::Run()
{
	std::deque<PSOCKET>::iterator it;

	int loopCount = 0;
	while (IsAlive()) {
		_TryAddNewTasks();

		if (tasks_.empty()) {
			// sleep 100 ms and continue
			std::this_thread::sleep_for(std::chrono::microseconds(100));
			continue;
		}

		const int nReady =
			poller_->Poll(firedEvents_, static_cast<int>(tasks_.size()), 1);
		for (int i = 0; i < nReady; ++i) {
			assert(!(firedEvents_[i].events & EventTypeWrite));

			Socket *sock = (Socket *)firedEvents_[i].userdata;

			if (firedEvents_[i].events & EventTypeRead) {
				if (!sock->OnReadable()) {
					sock->OnError();
				}
			}

			if (firedEvents_[i].events & EventTypeError) {
				sock->OnError();
			}
		}

		if (nReady == 0)
			loopCount *= 2;

		if (++loopCount < 100000)
			continue;

		loopCount = 0;
		for (auto it(tasks_.begin()); it != tasks_.end();) {
			if ((*it)->Invalid()) {
				NetThreadPool::Instance().DisableRead(*it);
				RemoveSocket(*it, EventTypeRead);
				it = tasks_.erase(it);
			} else {
				++it;
			}
		}
	}
}

void SendThread::Run()
{
	std::deque<PSOCKET>::iterator it;

	while (IsAlive()) {
		_TryAddNewTasks();

		size_t nOut = 0;
		for (it = tasks_.begin(); it != tasks_.end();) {
			Socket::SocketType type = (*it)->GetSocketType();
			Socket *sock = (*it).get();

			if (type == Socket::SocketType_Stream) {
				StreamSocket *tcpSock = static_cast<StreamSocket *>(sock);
				if (!tcpSock->Send())
					tcpSock->OnError();
			}

			if (sock->Invalid()) {
				NetThreadPool::Instance().DisableWrite(*it);
				RemoveSocket(*it, EventTypeWrite);
				it = tasks_.erase(it);
			} else {
				if (sock->epollOut_)
					++nOut;

				++it;
			}
		}

		if (nOut == 0) {
			std::this_thread::sleep_for(std::chrono::microseconds(100));
			continue;
		}

		const int nReady =
			poller_->Poll(firedEvents_, static_cast<int>(tasks_.size()), 1);
		for (int i = 0; i < nReady; ++i) {
			Socket *sock = (Socket *)firedEvents_[i].userdata;

			assert(!(firedEvents_[i].events & EventTypeRead));
			if (firedEvents_[i].events & EventTypeWrite) {
				if (!sock->OnWritable()) {
					sock->OnError();
				}
			}

			if (firedEvents_[i].events & EventTypeError) {
				sock->OnError();
			}
		}
	}
}

bool NetThreadPool::StopAllThreads()
{
	recvThread_->Stop();
	recvThread_.reset();
	sendThread_->Stop();
	sendThread_.reset();
}

bool NetThreadPool::AddSocket(PSOCKET sock, uint32_t events)
{
	if (events & EventTypeRead) {
		if (!recvThread_)
			return false;

		recvThread_->AddSocket(sock, EventTypeRead);
	}

	if (events & EventTypeWrite) {
		if (!sendThread_)
			return false;

		sendThread_->AddSocket(sock, EventTypeWrite);
	}

	return true;
}

bool NetThreadPool::StartAllThreads()
{
	recvThread_.reset(new RecvThread);
	sendThread_.reset(new SendThread);

	ThreadPool::Instance().ExecuteTask(std::bind(&RecvThread::Run, recvThread_));
	ThreadPool::Instance().ExecuteTask(std::bind(&SendThread::Run, sendThread_));

	return true;
}

void NetThreadPool::EnableRead(const std::shared_ptr<Socket> &sock)
{
	if (recvThread_)
		recvThread_->ModSocket(sock, EventTypeRead);
}

void NetThreadPool::EnableWrite(const std::shared_ptr<Socket> &sock)
{
	if (sendThread_)
		sendThread_->ModSocket(sock, EventTypeWrite);
}

void NetThreadPool::DisableRead(const std::shared_ptr<Socket> &sock)
{
	if (recvThread_)
		recvThread_->ModSocket(sock, 0);
}

void NetThreadPool::DisableWrite(const std::shared_ptr<Socket> &sock)
{
	if (sendThread_)
		sendThread_->ModSocket(sock, 0);
}

} // namespace Internal
