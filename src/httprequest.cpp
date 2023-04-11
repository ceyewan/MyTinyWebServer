#include "httprequest.h"
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <functional>
#include <mysql/mysql.h>
#include <string>

const std::unordered_set<std::string> HTTPRequest::DEFAULT_HTML{
    "/index", "/register", "/login", "/welcome", "error"};

const std::unordered_map<std::string, int> HTTPRequest::DEFAULT_HTML_TAG{
    {"/register.html", 0},
    {"/login.html", 1},
};

void HTTPRequest::Init() {
  method_ = path_ = version_ = body_ = "";
  state_ = REQUEST_LINE;
  header_.clear();
  post_.clear();
}

bool HTTPRequest::Parse(Buffer &buffer) {
  const char *CRLF = "\r\n"; // 换行符
  if (buffer.ReadableBytes() <= 0) {
    return false;
  }
  while (state_ != FINISH) {
    // 从 buffer 中找到换行符的地址
    char *lineEnd =
        std::search(buffer.CurReadPtr(), buffer.CurWritePtr(), CRLF, CRLF + 2);
    // 将这一行转化为 string
    std::string line(buffer.CurReadPtr(), lineEnd);
    switch (state_) {
    case REQUEST_LINE: // 解析请求行和路径
      if (!ParseRequestLine(line))
        return false;
      ParsePath();
      break;
    case HEADERS: // 解析请求头
      ParseHeader(line);
      if (buffer.ReadableBytes() <= 2) {
        state_ = FINISH;
      }
      break;
    case BODY: // 解析主体
      ParseBody(line);
      break;
    default:
      break;
    }
    if (lineEnd == buffer.CurWritePtr()) {
      break;
    }
    buffer.AddReadPtr(lineEnd + 2);
  }
  return true;
}

void HTTPRequest::ParsePath() {
  if (path_ == "/") {
    path_ = "/index.html";
  } else {
    for (auto &item : DEFAULT_HTML) {
      if (item == path_) {
        path_ += ".html";
        break;
      }
    }
  }
}

bool HTTPRequest::ParseRequestLine(const std::string &line) {
  std::regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
  std::smatch subMatch;
  if (regex_match(line, subMatch, patten)) {
    method_ = subMatch[1];
    path_ = subMatch[2];
    version_ = subMatch[3];
    state_ = HEADERS;
    return true;
  }
  return false;
}

void HTTPRequest::ParseHeader(const std::string &line) {
  std::regex patten("^([^:]*): ?(.*)$");
  std::smatch subMatch;
  if (regex_match(line, subMatch, patten)) {
    header_[subMatch[1]] = subMatch[2];
  } else {
    state_ = BODY;
  }
}

void HTTPRequest::ParseBody(const std::string &line) {
  body_ = line;
  // printf("\n%s\n", line.c_str()); // username=xxx&password=xxx
  if (method_ == "POST" &&
      header_["Content-Type"] == "application/x-www-form-urlencoded") {
    std::regex patten("username=(.*)&password=(.*)$");
    std::smatch subMatch;
    std::hash<std::string> strHash;
    if (regex_match(line, subMatch, patten)) {
      post_["username"] = subMatch[1];
      post_["password"] = std::to_string(strHash(subMatch[2]));
    } else {
      return;
    }
    if (DEFAULT_HTML_TAG.count(path_)) {
      int tag = DEFAULT_HTML_TAG.find(path_)->second;
      bool is_login = (tag == 1);
      if (UserVerify(post_["username"], post_["password"], is_login)) {
        path_ = "/welcome.html";
      } else {
        path_ = "/error.html";
      }
    }
  }
  state_ = FINISH;
}

bool HTTPRequest::UserVerify(const std::string &name, const std::string &pwd,
                             bool is_login) {
  if (name.empty() || pwd.empty())
    return false;
  MYSQL *sql;
  SqlConnRAII sqlconn(&sql, SqlConnPool::Instance());
  assert(sql);
  char order[256] = {0};
  MYSQL_RES *res = nullptr;
  snprintf(order, 256,
           "SELECT username, password FROM user WHERE username='%s' LIMIT 1",
           name.c_str());
  if (mysql_query(sql, order)) {
    mysql_free_result(res);
    return false;
  }
  res = mysql_store_result(sql);
  while (MYSQL_ROW row = mysql_fetch_row(res)) {
    std::string password(row[1]);
    if (is_login) {
      return pwd == password;
    }
    return false;
  }
  mysql_free_result(res);
  bzero(order, 256);
  snprintf(order, 256, "INSERT INTO user(username, password) VALUES('%s','%s')",
           name.c_str(), pwd.c_str());
  if (mysql_query(sql, order)) {
    mysql_free_result(res);
    return false;
  }
  return true;
}

__attribute__((unused)) std::string
HTTPRequest::GetPost(const std::string &key) {
  assert(!key.empty());
  if (post_.count(key) == 1) {
    return post_.find(key)->second;
  }
  return "";
}

bool HTTPRequest::IsKeepAlive() const {
  if (header_.count("Connection") == 1) {
    return header_.find("Connection")->second == "keep-alive" &&
           version_ == "1.1";
  }
  return false;
}