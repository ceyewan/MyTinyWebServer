#ifndef SQLCONNRAII_H
#define SQLCONNRAII_H

#include <cassert>
#include <cstddef>
#include <mutex>
#include <mysql/mysql.h>
#include <queue>
#include <semaphore.h>
#include <string>
#include <thread>

class SqlConnPool {
public:
  // 返回 SqlConnPool 实例，核心数据结构为 conn_queue_，其中有许多
  // sql，可以用来处理数据库查询
  static SqlConnPool *Instance();
  // 从 conn_queue_ 中拿到一个 sql
  MYSQL *GetConn();
  // 将 sql 释放回 conn_queue_
  void FreeConn(MYSQL *sql);
  size_t GetFreeConnCount();
  // 创建 connSize 个可以连接该特定数据库的 sql 实例
  void Init(const char *host, int port, const char *user, const char *pwd,
            const char *dbName, int connSize);
  void ClosePool();

private:
  SqlConnPool() = default;
  ~SqlConnPool() { ClosePool(); };

private:
  int MAX_CONN_{0};
  std::queue<MYSQL *> conn_queue_;
  std::mutex mutex_;
  sem_t sem_{};
};

/* 资源在对象构造初始化 资源在对象析构时释放*/
class SqlConnRAII {
public:
  // 对于每次调用 GetConn 都需要在使用完成后调用 FreeConn。因此利用 RAII
  SqlConnRAII(MYSQL **sql, SqlConnPool *conn_pool) {
    assert(conn_pool);
    *sql = conn_pool->GetConn();
    sql_ = *sql;
    connpool_ = conn_pool;
  }

  ~SqlConnRAII() {
    if (sql_) {
      connpool_->FreeConn(sql_);
    }
  }

private:
  MYSQL *sql_;
  SqlConnPool *connpool_;
};

#endif // SQLCONNRAII_H