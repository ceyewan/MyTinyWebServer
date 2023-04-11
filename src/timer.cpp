#include "timer.h"

void TimerManager::Delete(size_t i) {
  assert(0 <= i && i < heap_.size());
  size_t n = heap_.size() - 1;
  SwapNode(i, n);
  hash_.erase(heap_.back().id);
  heap_.pop_back();
  if (i < heap_.size()) {
    ShiftDown(i);
    ShiftUp(i);
  }
}

void TimerManager::SwapNode(size_t i, size_t j) {
  assert(0 <= i && i < heap_.size());
  assert(0 <= j && j < heap_.size());
  std::swap(heap_[i], heap_[j]);
  hash_[heap_[i].id] = i;
  hash_[heap_[j].id] = j;
}

void TimerManager::ShiftUp(size_t i) {
  assert(0 <= i && i < heap_.size());
  size_t j = (i - 1) / 2;
  if (heap_[i] < heap_[j]) {
    SwapNode(i, j);
    ShiftUp(j);
  }
}

void TimerManager::ShiftDown(size_t i) {
  assert(0 <= i && i < heap_.size());
  size_t j1 = i * 2 + 1, j2 = i * 2 + 2, j;
  if (j1 >= heap_.size())
    return;
  if (j2 >= heap_.size())
    j = j1;
  else
    j = heap_[j1] < heap_[j2] ? j1 : j2;
  if (heap_[j] < heap_[i]) {
    SwapNode(i, j);
    ShiftDown(j);
  }
}

void TimerManager::Adjust(int id, int newExpires) {
  assert(hash_.count(id) > 0 && hash_[id] < heap_.size() && 0 <= hash_[id]);
  heap_[hash_[id]].expires = Clock::now() + MS(newExpires);
  ShiftDown(hash_[id]);
  ShiftUp(hash_[id]);
}

void TimerManager::Add(int id, int timeOut, const TimeoutCallBack &cb) {
  assert(id >= 0);
  if (hash_.count(id) == 0) {
    size_t i = heap_.size();
    hash_[id] = i;
    heap_.push_back({id, Clock::now() + MS(timeOut), cb});
    ShiftUp(i);
  } else {
    size_t i = hash_[id];
    heap_[i].expires = Clock::now() + MS(timeOut);
    heap_[i].callback = cb;
    ShiftDown(i);
    ShiftUp(i);
  }
}

__attribute__((unused)) void TimerManager::Work(int id) {
  if (hash_.count(id) == 0 || heap_.empty())
    return;
  size_t i = hash_[id];
  heap_[i].callback();
  Delete(i);
}

void TimerManager::Tick() {
  while (!heap_.empty()) {
    TimerNode node = heap_.front();
    if (std::chrono::duration_cast<MS>(node.expires - Clock::now()).count() >
        0) {
      break;
    }
    node.callback();
    Pop();
  }
}

size_t TimerManager::GetNextTick() {
  Tick();
  size_t res = -1;
  if (!heap_.empty()) {
    res = std::chrono::duration_cast<MS>(heap_.front().expires - Clock::now())
              .count();
    if (res < 0)
      res = 0;
  }
  return res;
}

void TimerManager::Pop() {
  assert(!heap_.empty());
  Delete(0);
}