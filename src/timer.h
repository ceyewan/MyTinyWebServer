#ifndef TIMER_H
#define TIMER_H

#include <algorithm>
#include <arpa/inet.h>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <ctime>
#include <functional>
#include <iostream>
#include <ostream>
#include <queue>
#include <unordered_map>

typedef std::function<void()> TimeoutCallBack;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;
typedef Clock::time_point TimeStamp;

struct TimerNode {
  int id;
  TimeStamp expires;
  TimeoutCallBack callback;
  bool operator<(const TimerNode &t) const { return expires < t.expires; }
};

class TimerManager {
public:
  TimerManager() { heap_.reserve(64); };
  ~TimerManager() {
    heap_.clear();
    hash_.clear();
  }
  // 调整 id 的超时时间
  void Adjust(int id, int newExpires);
  // 添加一个连接，超时时间为 timeOut，回调函数为 cb，回调函数将在 Delete 时调用
  void Add(int id, int timeOut, const TimeoutCallBack &cb);
  // 调用回调函数后，删除
  __attribute__((unused)) void Work(int id);
  // 调用 Tick 将所有超时连接删除
  size_t GetNextTick();

private:
  // 将所有超时连接删除
  void Tick();
  // 删除堆顶
  void Pop();
  // 小根堆操作
  void Delete(size_t i);
  void ShiftDown(size_t i);
  void ShiftUp(size_t i);
  void SwapNode(size_t i, size_t j);

private:
  // 小根堆底层数据结构
  std::vector<TimerNode> heap_;
  // id 到 TimerNode 的映射
  std::unordered_map<int, size_t> hash_;
};

#endif // TIMER_H
