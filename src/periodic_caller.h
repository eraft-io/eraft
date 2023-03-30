/**
 * @file periodic_caller.h
 * @author ERaftGroup
 * @brief
 * @version 0.1
 * @date 2023-03-30
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <chrono>
#include <cstdio>
#include <functional>
#include <future>

/**
 * @brief
 *
 */
class PeriodicCaller {
 public:
  /**
   * @brief Construct a new Periodic Caller object
   *
   * @tparam callable
   * @tparam arguments
   * @param interval
   * @param async
   * @param f
   * @param args
   */
  template <class callable, class... arguments>
  PeriodicCaller(int64_t    interval,
                 bool       async,
                 callable&& f,
                 arguments&&... args) {
    std::function<typename std::result_of<callable(arguments...)>::type()> task(
        std::bind(std::forward<callable>(f), std::forward<arguments>(args)...));
    if (async) {

      while (1) {
        std::thread([interval, task]() {
          std::this_thread::sleep_for(std::chrono::milliseconds(interval));
          task();
        }).detach();
      }

    } else {
      while (1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(interval));
        task();
      }
    }
  }
};