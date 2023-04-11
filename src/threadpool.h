#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <cassert>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>

class ThreadPool {
public:
  // 构造函数，构造后台守护进程
  explicit ThreadPool(size_t thread_count = 8);
  ThreadPool() = default;
  ThreadPool(ThreadPool &&) = default;
  ~ThreadPool();
  // 向进程池中添加任务
  template <class T> void AddTask(T &&task);

private:
  struct Pool {
    std::mutex mutex;
    std::condition_variable cond;
    bool close{};
    std::queue<std::function<void()>> tasks;
  };
  std::shared_ptr<Pool> pool_;
};

inline ThreadPool::ThreadPool(size_t thread_count)
    : pool_(std::make_shared<Pool>()) {
  assert(thread_count > 0);
  for (size_t i = 0; i < thread_count; i++) {
    std::thread([pool = pool_] {
      std::unique_lock<std::mutex> locker(pool->mutex);
      while (true) {
        if (!pool->tasks.empty()) {
          auto task = std::move(pool->tasks.front());
          pool->tasks.pop();
          locker.unlock();
          task();
          locker.lock();
        } else if (pool->close)
          break;
        else
          pool->cond.wait(locker);
      }
    }).detach();
  }
}

inline ThreadPool::~ThreadPool() {
  if (static_cast<bool>(pool_)) {
    pool_->mutex.lock();
    pool_->close = true;
    pool_->mutex.unlock();
    pool_->cond.notify_all();
  }
}

template <class F> void ThreadPool::AddTask(F &&task) {
  {
    std::lock_guard<std::mutex> locker(pool_->mutex);
    pool_->tasks.emplace(std::forward<F>(task));
  }
  pool_->cond.notify_one();
}

#endif // THREADPOOL_H