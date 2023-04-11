#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include <arpa/inet.h> // sockaddr_in
#include <cerrno>
#include <cstdlib> // atoi()
#include <sys/types.h>
#include <sys/uio.h> // readv/writev

#include "httprequest.h"
#include "httpresponse.h"

using namespace std;

class HTTPConn {
public:
  HTTPConn() = default;
  ~HTTPConn() { Close(); };
  void Init(int sockFd, const sockaddr_in &addr);
  ssize_t Read(int *saveErrno);
  ssize_t Write(int *saveErrno);
  bool Process();
  void Close();
  int GetFd() const { return fd_; };
  __attribute__((unused)) int GetPort() const { return addr_.sin_port; };
  __attribute__((unused)) const char *GetIP() const {
    return inet_ntoa(addr_.sin_addr);
  };
  __attribute__((unused)) sockaddr_in GetAddr() const { return addr_; };
  size_t ToWriteBytes() { return iov_[0].iov_len + iov_[1].iov_len; }
  bool IsKeepAlive() const { return request_.IsKeepAlive(); }

public:
  static bool isET;
  static const char *srcDir;
  static std::atomic<int> userCount;

private:
  int fd_{-1};
  struct sockaddr_in addr_ {
    0
  };
  bool is_close_{true};
  int iov_cnt_{};
  struct iovec iov_[2]{};
  Buffer read_buffer_;  // 读缓冲区
  Buffer write_buffer_; // 写缓冲区
  HTTPRequest request_;
  HTTPResponse response_;
};

#endif // HTTP_CONN_H