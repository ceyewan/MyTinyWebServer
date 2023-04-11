#include "httpconn.h"
#include <cerrno>
#include <cstdio>
#include <sys/uio.h>
#include <unistd.h>

const char *HTTPConn::srcDir;
std::atomic<int> HTTPConn::userCount;
bool HTTPConn::isET;

void HTTPConn::Init(int sockFd, const sockaddr_in &addr) {
  userCount++;
  addr_ = addr;
  fd_ = sockFd;
  write_buffer_.Init();
  read_buffer_.Init();
  is_close_ = false;
}

void HTTPConn::Close() {
  response_.UnmapFile();
  if (!is_close_) {
    userCount--;
    is_close_ = true;
    close(fd_);
  }
}

ssize_t HTTPConn::Read(int *saveErrno) {
  ssize_t size = -1;
  do {
    size = read_buffer_.ReadFd(fd_, saveErrno);
    if (size <= 0)
      break;
  } while (isET);
  return size;
}

ssize_t HTTPConn::Write(int *saveErrno) {
  ssize_t size = -1;
  do {
    size = writev(fd_, iov_, iov_cnt_);
    if (size <= 0) {
      *saveErrno = errno;
      break;
    } else if (iov_[0].iov_len + iov_[1].iov_len == 0) {
      break;
    } else if (static_cast<size_t>(size) > iov_[0].iov_len) {
      iov_[1].iov_base = (uint8_t *)iov_[1].iov_base + (size - iov_[0].iov_len);
      iov_[1].iov_len -= (size - iov_[0].iov_len);
      if (iov_[0].iov_len) {
        write_buffer_.Init();
        iov_[0].iov_len = 0;
      }
    } else {
      iov_[0].iov_base = (uint8_t *)iov_[0].iov_base + size;
      iov_[0].iov_len -= size;
      write_buffer_.AddReadPtr(size);
    }
  } while (isET || ToWriteBytes() > 0);
  return size;
}

bool HTTPConn::Process() {
  request_.Init();
  if (read_buffer_.ReadableBytes() <= 0) {
    return false;
  } else if (request_.Parse(read_buffer_)) {
    response_.Init(srcDir, request_.Path(), request_.IsKeepAlive(), 200);
  } else {
    response_.Init(srcDir, request_.Path(), false, 400);
  }
  response_.MakeResponse(write_buffer_);
  iov_[0].iov_base = const_cast<char *>(write_buffer_.CurReadPtr());
  iov_[0].iov_len = write_buffer_.ReadableBytes();
  iov_cnt_ = 1;
  if (response_.File() && response_.FileLen()) {
    iov_[1].iov_base = response_.File();
    iov_[1].iov_len = response_.FileLen();
    iov_cnt_ = 2;
  }
  return true;
}