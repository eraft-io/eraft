#include "thread_pool.h"

__thread bool ThreadPool::working_ = true;

ThreadPool::ThreadPool() : waiters_(0), shutdown_(false)
{
	monitor_ = std::thread([this]() { this->_MonitorRoutine(); });
	maxIdleThread_ = std::max(1U, std::thread::hardware_concurrency());
	pendingStopSignal_ = 0;
}

ThreadPool::~ThreadPool()
{
	JoinAll();
}

ThreadPool &ThreadPool::Instance()
{
	static ThreadPool pool;
	return pool;
}

void ThreadPool::SetMaxIdleThread(unsigned int m)
{
	if (0 < m && m <= kMaxThreads) {
		maxIdleThread_ = m;
	}
}

void ThreadPool::JoinAll()
{
	decltype(workers_) tmp;

	{
		std::unique_lock<std::mutex> guard(mutex_);
		if (shutdown_)
			return;

		shutdown_ = true;
		cond_.notify_all();
		tmp.swap(workers_);
		workers_.clear();
	}

	for (auto &t : tmp) {
		if (t.joinable())
			t.join();
	}

	if (monitor_.joinable()) {
		monitor_.join();
	}
}

void ThreadPool::_CreateWorker()
{
	std::thread t([this]() { this->_WorkerRoutine(); });

	workers_.push_back(std::move(t));
}

void ThreadPool::_WorkerRoutine()
{
	working_ = true;

	while (working_) {
		std::function<void()> task;

		{
			std::unique_lock<std::mutex> gurad(mutex_);

			++waiters_;
			cond_.wait(gurad, [this]() -> bool {
				// when shutdown or tasks is not empty, thread continue
				return this->shutdown_ ||
					!tasks_.empty(); // which returns ​false if the
							 // waiting should be continued
			});
			--waiters_;

			if (this->shutdown_ && tasks_.empty())
				return;

			task = std::move(tasks_.front());
			tasks_.pop_front();
		}
		task();
	}

	--pendingStopSignal_;
}

void ThreadPool::_MonitorRoutine()
{
	while (!shutdown_) {
		std::this_thread::sleep_for(std::chrono::seconds(1));

		std::unique_lock<std::mutex> gurad(mutex_);
		if (shutdown_)
			return;

		auto nw = waiters_;

		nw -= pendingStopSignal_;

		while (nw-- > maxIdleThread_) {
			tasks_.push_back([this]() { working_ = false; });
			cond_.notify_one(); // notify_one unblocks one of the waiting
					    // threads
			++pendingStopSignal_;
		}
	}
}
