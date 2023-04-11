#include "webserver.h"
#include <cstdio>

WebServer::WebServer(int port, int trigMode, int timeoutMS, int sqlPort,
                     const char *sqlUser, const char *sqlPwd,
                     const char *dbName, int connPoolNum, int threadNum)
    : port_(port), timeoutMS_(timeoutMS), is_close_(false),
      timer_(new TimerManager()), threadpool_(new ThreadPool(threadNum)),
      epoller_(new Epoller()) {
  src_dir_ = getcwd(nullptr, 256);
  assert(src_dir_);
  strncat(src_dir_, "/resources/", 16);
  HTTPConn::userCount = 0;
  HTTPConn::srcDir = src_dir_;
  SqlConnPool::Instance()->Init("localhost", sqlPort, sqlUser, sqlPwd, dbName,
                                connPoolNum);
  InitEventMode(trigMode);
  if (!InitSocket()) {
    is_close_ = true;
  }
}

WebServer::~WebServer() {
  close(listenFd_);
  is_close_ = true;
  free(src_dir_);
  SqlConnPool::Instance()->ClosePool();
}

void WebServer::InitEventMode(int trigMode) {
  listen_event_ = EPOLLRDHUP;
  conn_event_ = EPOLLONESHOT | EPOLLRDHUP;
  switch (trigMode) {
  case 0:
    break;
  case 1:
    conn_event_ |= EPOLLET;
    break;
  case 2:
    listen_event_ |= EPOLLET;
    break;
  case 3:
    listen_event_ |= EPOLLET;
    conn_event_ |= EPOLLET;
    break;
  default:
    assert(0);
  }
  HTTPConn::isET = (conn_event_ & EPOLLET);
}

void WebServer::Start() {
  int timeMS = -1; /* epoll wait timeout == -1 无事件将阻塞 */
  while (!is_close_) {
    if (timeoutMS_ > 0) {
      timeMS = static_cast<int>(timer_->GetNextTick());
    }
    int eventCnt = epoller_->Wait(timeMS);
    for (int i = 0; i < eventCnt; i++) {
      int fd = epoller_->GetEventFd(i);
      uint32_t events = epoller_->GetEvents(i);
      if (fd == listenFd_) {
        DealListen();
      } else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
        assert(users_.count(fd) > 0);
        CloseConn(&users_[fd]);
      } else if (events & EPOLLIN) {
        assert(users_.count(fd) > 0);
        DealRead(&users_[fd]);
      } else if (events & EPOLLOUT) {
        assert(users_.count(fd) > 0);
        DealWrite(&users_[fd]);
      } else {
        abort();
      }
    }
  }
}

void WebServer::CloseConn(HTTPConn *client) {
  assert(client);
  epoller_->DelFd(client->GetFd());
  client->Close();
}

void WebServer::AddClient(int fd, sockaddr_in addr) {
  assert(fd > 0);
  users_[fd].Init(fd, addr);
  if (timeoutMS_ > 0) {
    timer_->Add(fd, timeoutMS_,
                [this, capture0 = &users_[fd]] { CloseConn(capture0); });
  }
  epoller_->AddFd(fd, EPOLLIN | conn_event_);
  SetFdNonblock(fd);
}

void WebServer::DealListen() {
  struct sockaddr_in addr {};
  socklen_t len = sizeof(addr);
  do {
    int fd = accept(listenFd_, (struct sockaddr *)&addr, &len);
    if (fd <= 0) {
      return;
    } else if (HTTPConn::userCount >= MAX_FD) {
      SendError(fd, "Server busy!");
      return;
    }
    AddClient(fd, addr);
  } while (listen_event_ & EPOLLET);
}

void WebServer::DealRead(HTTPConn *client) {
  assert(client);
  ExtentTime(client);
  threadpool_->AddTask([this, client] { OnRead(client); });
}

void WebServer::DealWrite(HTTPConn *client) {
  assert(client);
  ExtentTime(client);
  threadpool_->AddTask([this, client] { OnWrite(client); });
}

void WebServer::ExtentTime(HTTPConn *client) {
  assert(client);
  if (timeoutMS_ > 0) {
    timer_->Adjust(client->GetFd(), timeoutMS_);
  }
}

void WebServer::OnRead(HTTPConn *client) {
  assert(client);
  ssize_t ret = -1;
  int readErrno = 0;
  ret = client->Read(&readErrno);
  if (ret <= 0 && readErrno != EAGAIN) {
    CloseConn(client);
    return;
  }
  OnProcess(client);
}

void WebServer::OnProcess(HTTPConn *client) {
  if (client->Process()) {
    epoller_->ModFd(client->GetFd(), conn_event_ | EPOLLOUT);
  } else {
    epoller_->ModFd(client->GetFd(), conn_event_ | EPOLLIN);
  }
}

void WebServer::OnWrite(HTTPConn *client) {
  assert(client);
  ssize_t ret = -1;
  int writeErrno = 0;
  ret = client->Write(&writeErrno);
  if (client->ToWriteBytes() == 0) {
    if (client->IsKeepAlive()) {
      OnProcess(client);
      return;
    }
  } else if (ret < 0) {
    if (writeErrno == EAGAIN) {
      epoller_->ModFd(client->GetFd(), conn_event_ | EPOLLOUT);
      return;
    }
  }
  CloseConn(client);
}

int WebServer::SetFdNonblock(int fd) {
  assert(fd > 0);
  return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}

bool WebServer::InitSocket() {
  int ret;
  struct sockaddr_in addr {};
  if (port_ > 65535 || port_ < 1024) {
    return false;
  }
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port_);
  listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (listenFd_ < 0) {
    return false;
  }
  int optval = 1;
  ret = setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval,
                   sizeof(int));
  if (ret == -1) {
    close(listenFd_);
    return false;
  }
  ret = bind(listenFd_, (struct sockaddr *)&addr, sizeof(addr));
  if (ret < 0) {
    close(listenFd_);
    return false;
  }
  ret = listen(listenFd_, SOMAXCONN);
  if (ret < 0) {
    close(listenFd_);
    return false;
  }
  ret = epoller_->AddFd(listenFd_, listen_event_ | EPOLLIN);
  if (ret == 0) {
    close(listenFd_);
    return false;
  }
  SetFdNonblock(listenFd_);
  return true;
}

void WebServer::SendError(int fd, const char *info) {
  assert(fd > 0);
  ssize_t ret = send(fd, info, strlen(info), 0);
  assert(ret >= 0);
  close(fd);
}
