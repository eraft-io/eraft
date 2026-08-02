#pragma once
// util.h — Timers, BlockingQueue, debug, constants
// Corresponds to Go: util.go

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdarg>
#include <cstdio>
#include <mutex>
#include <queue>
#include <random>
#include <vector>

namespace raft {

// ── Debug flag ───────────────────────────────────────────────
constexpr bool kDebug = false;

inline void DPrintf(const char* fmt, ...) {
    if (!kDebug) return;
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
}

// ── Timeout constants ────────────────────────────────────────
constexpr int kHeartbeatTimeoutMs = 125;
constexpr int kElectionTimeoutMs  = 1000;

inline std::chrono::milliseconds StableHeartbeatTimeout() {
    return std::chrono::milliseconds(kHeartbeatTimeoutMs);
}

inline std::chrono::milliseconds RandomizedElectionTimeout() {
    thread_local std::mt19937 rng(
        std::random_device{}() ^
        static_cast<unsigned>(std::chrono::steady_clock::now()
                                   .time_since_epoch().count()));
    std::uniform_int_distribution<int> dist(0, kElectionTimeoutMs - 1);
    return std::chrono::milliseconds(kElectionTimeoutMs + dist(rng));
}

// ── insertion_sort (descending, for commitIndex calc) ────────
inline void insertion_sort_desc(std::vector<int>& v) {
    for (size_t i = 1; i < v.size(); ++i) {
        for (size_t j = i; j > 0 && v[j] > v[j - 1]; --j) {
            std::swap(v[j], v[j - 1]);
        }
    }
}

// ── Timer ────────────────────────────────────────────────────
// Mimics Go's time.Timer with Reset() and Stop().
// A dedicated thread sleeps on a condition_variable and fires
// the callback when the deadline is reached.
class Timer {
public:
    Timer() = default;
    ~Timer() { Stop(); }

    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;

    // Start / restart the timer with the given duration.
    // When it fires, it calls the stored callback exactly once
    // (then blocks until Reset() is called again).
    void Reset(std::chrono::milliseconds duration) {
        std::lock_guard<std::mutex> lk(mu_);
        deadline_ = std::chrono::steady_clock::now() + duration;
        fired_    = false;
        running_  = true;
        cv_.notify_all();
    }

    // Cancel the timer.  Safe to call multiple times.
    void Stop() {
        std::lock_guard<std::mutex> lk(mu_);
        running_ = false;
        cv_.notify_all();
    }

    // Block the caller thread until the timer fires or Stop() is called.
    // Returns true if it fired, false if stopped.
    bool Wait() {
        std::unique_lock<std::mutex> lk(mu_);
        cv_.wait(lk, [&] {
            return !running_ ||
                   (!fired_ &&
                    std::chrono::steady_clock::now() >= deadline_);
        });
        if (running_ && !fired_) {
            fired_ = true;
            running_ = false;  // one-shot: mark as no longer running
            return true;  // fired
        }
        return false;     // stopped or already fired
    }

    // Non-blocking check: has the deadline passed since last Reset()?
    // Returns true at most once per Reset() call.
    bool Fired() {
        std::lock_guard<std::mutex> lk(mu_);
        if (!running_ && !fired_) return false;  // stopped
        if (fired_) return true;                  // already marked as fired
        if (running_ && std::chrono::steady_clock::now() >= deadline_) {
            fired_ = true;
            running_ = false;
            return true;
        }
        return false;
    }

    // Debug: get timer state
    struct TimerState { bool running; bool fired; int ms_until_deadline; };
    TimerState GetState() const {
        std::lock_guard<std::mutex> lk(mu_);
        auto now = std::chrono::steady_clock::now();
        int ms = static_cast<int>(
            std::chrono::duration_cast<std::chrono::milliseconds>(deadline_ - now).count());
        return {running_, fired_, ms};
    }

private:
    mutable std::mutex mu_;
    std::condition_variable cv_;
    std::chrono::steady_clock::time_point deadline_{};
    bool running_ = false;
    bool fired_   = true;   // start in "fired" so Wait() blocks until Reset()
};

// ── BlockingQueue<T> ─────────────────────────────────────────
// Thread-safe unbounded queue, replaces Go's buffered channel.
// pop() blocks until an element is available or Close() is called.
template <typename T>
class BlockingQueue {
public:
    void push(T val) {
        std::lock_guard<std::mutex> lk(mu_);
        if (closed_) return;
        q_.push(std::move(val));
        cv_.notify_one();
    }

    // Returns false when queue is closed AND empty.
    bool pop(T& out) {
        std::unique_lock<std::mutex> lk(mu_);
        cv_.wait(lk, [&] { return !q_.empty() || closed_; });
        if (q_.empty()) return false;
        out = std::move(q_.front());
        q_.pop();
        return true;
    }

    // Timed pop — returns false on timeout or closed+empty.
    template <typename Duration>
    bool pop_for(T& out, Duration timeout) {
        std::unique_lock<std::mutex> lk(mu_);
        if (!cv_.wait_for(lk, timeout, [&] { return !q_.empty() || closed_; })) {
            return false; // timeout
        }
        if (q_.empty()) return false;
        out = std::move(q_.front());
        q_.pop();
        return true;
    }

    void close() {
        std::lock_guard<std::mutex> lk(mu_);
        closed_ = true;
        cv_.notify_all();
    }

    bool empty() const {
        std::lock_guard<std::mutex> lk(mu_);
        return q_.empty();
    }

    // Non-blocking pop — returns false if queue is empty.
    bool try_pop(T& out) {
        std::lock_guard<std::mutex> lk(mu_);
        if (q_.empty()) return false;
        out = std::move(q_.front());
        q_.pop();
        return true;
    }

private:
    mutable std::mutex mu_;
    std::condition_variable cv_;
    std::queue<T> q_;
    bool closed_ = false;
};

} // namespace raft
