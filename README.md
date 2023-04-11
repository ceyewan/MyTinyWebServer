### 功能

-   使用标准库容器 vector 封装 char，实现自动增长的高性能缓冲区
-   使用 IO 多路复用 Epoll 与线程池、SQL 连接池实现多线程高并发的 Reactor 模型
-   基于小根堆实现定时器，关闭超时的非活动连接
-   利用正则表达式和状态机解析 HTTP 报文，支持 GET、POST 两种请求方式
-   利用 RAII 机制实现数据库连接池，减少数据库连接与断开的开销
-   使用 `getopt_long()` 解析命令行参数，支持使用长、短选项指定参数值
-   对 POST 请求字段做了防 SQL 注入检查，将密码进行哈希加密后存储

### 概述

```
.
├── bin				
│   └── server		server 可执行文件
├── CMakeLists.txt
├── LICENSE
├── Makefile
├── README.md
├── resources		资源文件
│   ├── error.html
│...│
│   └── welcome.html
├── src				代码源文件
│   ├── buffer.cpp
│   ├── buffer.h
│...│
│   ├── webserver.cpp
│   └── webserver.h
├── test  				测试文件代码
│...│
│   ├── webbench
│   └── webbench.c
└── test.sh				测试脚本
```

### 安装配置

#### 配置数据库

```mysql
create database ceyewan;
use ceyewan;
CREATE TABLE user(
    username char(50) NULL,
    password char(50) NULL
)ENGINE=InnoDB;
```

#### 编译执行

```
make
./bin/server -h # 查看选项参数
Options:
     -p --Port           server 监听端口, 默认为 8888
     -m --TrigMode       工作模式, 默认为 3, ET 模式
     -t --TimeoutMS      超时时间, 默认为 6000
     -s --SQLport        MySQL 的端口, 默认 3306
     -u --SQLuser        MySQL 的用户, 默认 root
     -k --SQLkey         MySQL 的密码, 默认为 password
     -d --Database       MySQL 数据库, 默认为 ceyewan
     -n --ThreadNums     线程池数量, 默认为 8
     -c --ConnPoolNums   SQL 连接池数量, 默认为 8
     -h --help           帮助
```

#### 压力测试

```shell
./test.sh
```

![img](https://image.ceyewan.top/typora/image-20230411215706904.png)

小破笔记本中的 wsl 测试，大概 1w QPS。

### 在线演示

```
http://ceyewan.top:8888
```

### TODO

-   日志系统
-   Cookie 认证

### 参考

- 牛客 C++ 高性能服务器开发项目

- [github 参考项目](https://github.com/markparticle/WebServer)
