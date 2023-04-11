#include "buffer.h"

Buffer::Buffer(int initBuffSize) { buffer_.resize(initBuffSize); }

void Buffer::Init() {
  bzero(&buffer_[0], buffer_.size());
  read_pos_ = write_pos_ = 0;
}

std::string Buffer::AllToStr() {
  std::string str(CurReadPtr(), ReadableBytes());
  Init();
  return str;
}

ssize_t Buffer::ReadFd(int fd, int *Errno) {
  char buff[65535];
  struct iovec iov[2];
  const size_t writeable = WriteableBytes();
  iov[0].iov_base = CurWritePtr();
  iov[0].iov_len = writeable;
  iov[1].iov_base = buff;
  iov[1].iov_len = sizeof(buff);
  const ssize_t size = readv(fd, iov, 2);
  if (size < 0) {
    *Errno = errno;
  } else if (static_cast<size_t>(size) <= writeable) {
    write_pos_ += size;
  } else {
    write_pos_ += writeable;
    Append(buff, size - writeable);
  }
  return size;
}

__attribute__((unused)) ssize_t Buffer::WriteFd(int fd, int *Errno) {
  size_t readable_size = write_pos_ - read_pos_;
  ssize_t size = write(fd, CurReadPtr(), readable_size);
  if (size < 0) {
    *Errno = errno;
  } else {
    read_pos_ += size;
  }
  return size;
}

void Buffer::Append(const char *str, size_t size) {
  if (buffer_.size() - write_pos_ < size) {
    if (buffer_.size() - write_pos_ + read_pos_ < size) {
      buffer_.resize(write_pos_ + size + 1);
    } else {
      size_t tmp = CurWritePtr() - CurReadPtr();
      std::copy(CurReadPtr(), CurWritePtr(), BeginPtr());
      read_pos_ = 0;
      write_pos_ = read_pos_ + tmp;
    }
  }
  std::copy(str, str + size, CurWritePtr());
  write_pos_ += size;
}

void Buffer::Append(const std::string &str) { Append(str.c_str(), str.size()); }