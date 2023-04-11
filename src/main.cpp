#include "opt.h"
#include "webserver.h"
#include <unistd.h>

int main(int argc, char *argv[]) {
  /* 守护进程 后台运行 */
  // daemon(1, 0);

  GetOpt opt(argc, argv);

  WebServer server(
      opt.Port(), opt.TrigMode(), opt.TimeoutMS(), /* 端口 ET模式 timeoutMs */
      opt.sqlPort(), opt.sqlUser(), opt.SQLkey(), opt.dbName(), /* Mysql配置 */
      opt.ConnPoolNums(), opt.ThreadNums()); /* 连接池数量 线程池数量 */
  server.Start();
}