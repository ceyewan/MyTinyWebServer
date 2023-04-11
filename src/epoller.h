#ifndef EPOLLER_H
#define EPOLLER_H

#include <cassert>
#include <cerrno>
#include <cstddef>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

class Epoller {
public:
  // 对 epollfd_ 初始化，event_ 大小为1024
  explicit Epoller(int maxEvent = 1024);
  // 析构函数，close(epoll_)
  ~Epoller();
  // 将 fd 加入 epoll 的监控，监控类型为 events
  bool AddFd(int fd, u_int32_t events) const;
  // 将 fd 的监控类型修改为 events
  bool ModFd(int fd, u_int32_t events) const;
  // 将 fd 移除 epoll 的监控
  bool DelFd(int fd) const;
  // 检测所有的文件描述符，返回发生变化的文件描述符的数量，默认为非阻塞
  int Wait(int timeoutMs = -1);
  // 返回第 i 个 event 的 fd 和 events
  int GetEventFd(size_t i) const;
  u_int32_t GetEvents(size_t i) const;

private:
  int epollfd_;
  std::vector<struct epoll_event> events_;
};

#endif // EPOLLER_H