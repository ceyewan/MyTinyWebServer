#ifndef BUFFER_H
#define BUFFER_H

#include <algorithm>
#include <atomic>
#include <bits/types/struct_iovec.h>
#include <cassert>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <strings.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <vector>

class Buffer {
public:
  explicit Buffer(int initBuffSize = 1024);
  ~Buffer() = default;
  void Init();
  // 将可读部分的数据转化为 string
  std::string AllToStr();
  // 从 fd 中读取数据，为了防止空间不够，使用分散读 readv
  ssize_t ReadFd(int fd, int *Errno);
  // 将 buffer_ read 到 write 中的数据写入 fd 中
  __attribute__((unused)) ssize_t WriteFd(int fd, int *Errno);
  // 向 fd 中写入 str 数组，长度为 size，空间不够需要扩容
  void Append(const char *str, size_t size);
  // 重载 Append
  void Append(const std::string &str);
  // 可读的区间大小
  size_t ReadableBytes() const { return write_pos_ - read_pos_; }
  // 可写的区间大小
  size_t WriteableBytes() const { return buffer_.size() - write_pos_; }
  char *BeginPtr() { return &*buffer_.begin(); }
  char *CurReadPtr() { return BeginPtr() + read_pos_; }
  char *CurWritePtr() { return BeginPtr() + write_pos_; }
  // 将 read_pos_ 移动到特定指针位置
  void AddReadPtr(char *end) { read_pos_ += end - CurReadPtr(); }
  // 讲 read_pos_ 移动指定大小
  void AddReadPtr(size_t size) { read_pos_ += size; }

private:
  std::vector<char> buffer_;
  std::atomic<size_t> read_pos_{};
  std::atomic<size_t> write_pos_{};
};

#endif // BUFFER_H