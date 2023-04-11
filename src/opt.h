#pragma once

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <getopt.h>
#include <string>
#include <sys/types.h>
#include <unistd.h>

static struct option long_options[] = {
    {"Port", required_argument, nullptr, 'p'},
    {"TrigMode", required_argument, nullptr, 'm'},
    {"TimeoutMS", required_argument, nullptr, 't'},
    {"SQLport", required_argument, nullptr, 's'},
    {"SQLuser", required_argument, nullptr, 'u'},
    {"SQLkey", required_argument, nullptr, 'k'},
    {"Database", required_argument, nullptr, 'd'},
    {"ThreadNums", required_argument, nullptr, 'n'},
    {"ConnPoolNums", required_argument, nullptr, 'c'},
    {"Help", no_argument, nullptr, 'h'},
};

class GetOpt {
public:
  GetOpt(int argc, char *argv[]);
  void SwitchOpt(int opt);

  int Port() const { return port_; }
  int TrigMode() const { return trig_mode_; }
  int TimeoutMS() const { return timeoutMS_; }
  int sqlPort() const { return sql_port_; }
  int ThreadNums() const { return thread_nums_; }
  int ConnPoolNums() const { return conn_pool_nums_; }
  const char *sqlUser() { return sql_user_.c_str(); }
  const char *SQLkey() { return sql_key_.c_str(); }
  const char *dbName() const { return db_name_.c_str(); }

private:
  int port_{8888};
  int trig_mode_{3};
  int timeoutMS_{6000};
  int sql_port_{3306};
  std::string sql_user_{"root"};
  std::string sql_key_{"password"};
  std::string db_name_{"database"};
  int conn_pool_nums_{8};
  int thread_nums_{8};
};

void ShowHelp() {
  printf("Options:\n \
    -p --Port           server 监听端口, 默认为 9114\n \
    -m --TrigMode       工作模式, 默认为 3, ET 模式\n \
    -t --TimeoutMS      超时时间, 默认为 6000\n \
    -s --SQLport        MySQL 的端口, 默认 3306\n \
    -u --SQLuser        MySQL 的用户, 默认 root\n \
    -k --SQLkey         MySQL 的密码, 默认为 password\n \
    -d --Database       MySQL 数据库, 默认为 ceyewan\n \
    -n --ThreadNums     线程池数量, 默认为 8\n \
    -c --ConnPoolNums   SQL 连接池数量, 默认为 8\n \
    -h --help           帮助\n");
}

void GetOpt::SwitchOpt(int opt) {
  switch (opt) {
  case 'p':
    assert(optarg != nullptr);
    port_ = atoi(optarg);
    break;
  case 'm':
    assert(optarg != nullptr);
    trig_mode_ = atoi(optarg);
    break;
  case 't':
    assert(optarg != nullptr);
    timeoutMS_ = atoi(optarg);
    break;
  case 's':
    assert(optarg != nullptr);
    sql_port_ = atoi(optarg);
    break;
  case 'u':
    assert(optarg != nullptr);
    sql_user_ = optarg;
    break;
  case 'k':
    assert(optarg != nullptr);
    sql_key_ = optarg;
    break;
  case 'd':
    assert(optarg != nullptr);
    db_name_ = optarg;
    break;
  case 'n':
    assert(optarg != nullptr);
    thread_nums_ = atoi(optarg);
    break;
  case 'c':
    assert(optarg != nullptr);
    conn_pool_nums_ = atoi(optarg);
    break;
  case 'h':
    ShowHelp();
    exit(-1);
  default:
    printf("Please input legal flag!\n");
    ShowHelp();
    exit(-1);
  }
}

GetOpt::GetOpt(int argc, char *argv[]) {
  int opt;
  while ((opt = getopt_long(argc, argv, "p:m:t:s:u:k:d:n:c:h", long_options,
                            nullptr)) != -1) {
    SwitchOpt(opt);
  }
}