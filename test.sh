#!/bin/bash

# 网站 URL
url="http://127.0.0.1:9114/index.html"

# 并发数初始值
concurrency=50

# 循环次数
loops=10

# 压测命令
command="./test/webbench -c {concurrency} -t 30 {url}"

for i in $(seq 1 $loops); do
  # 替换占位符
  eval_command="${command//\{concurrency\}/$concurrency}"
  eval_command="${eval_command//\{url\}/$url}"
  # 执行压测命令
  eval "$eval_command"
  # 并发数加倍
  concurrency=$((concurrency * 2))
done
