#include "httpresponse.h"
#include "buffer.h"
#include <sys/stat.h>

const std::unordered_map<std::string, std::string> HTTPResponse::SUFFIX_TYPE = {
    {".html", "text/html"},
    {".xml", "text/xml"},
    {".xhtml", "application/xhtml+xml"},
    {".txt", "text/plain"},
    {".rtf", "application/rtf"},
    {".pdf", "application/pdf"},
    {".word", "application/nsword"},
    {".png", "image/png"},
    {".gif", "image/gif"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".au", "audio/basic"},
    {".mpeg", "video/mpeg"},
    {".mpg", "video/mpeg"},
    {".avi", "video/x-msvideo"},
    {".gz", "application/x-gzip"},
    {".tar", "application/x-tar"},
    {".css", "text/css "},
    {".js", "text/javascript "},
};

const std::unordered_map<int, std::string> HTTPResponse::CODE_STATUS = {
    {200, "OK"},
    {400, "Bad Request"},
    {403, "Forbidden"},
    {404, "Not Found"},
};

const std::unordered_map<int, std::string> HTTPResponse::CODE_PATH = {
    {400, "/error.html"},
    {403, "/error.html"},
    {404, "/error.html"},
};

void HTTPResponse::Init(const std::string &srcDir, std::string &path,
                        bool isKeepAlive, int code) {
  code_ = code;
  is_keep_alive_ = isKeepAlive;
  path_ = path;
  src_dir_ = srcDir;
}

void HTTPResponse::MakeResponse(Buffer &buffer) {
  if (stat((src_dir_ + path_).c_str(), &file_stat_) < 0 ||
      S_ISDIR(file_stat_.st_mode)) {
    code_ = 404;
  } else if (!(file_stat_.st_mode & S_IROTH)) {
    code_ = 403;
  } else if (code_ == -1) {
    code_ = 200;
  }
  ErrorHtml();
  AddStateLine(buffer);
  AddHeader(buffer);
  AddContent(buffer);
}

void HTTPResponse::ErrorHtml() {
  if (CODE_PATH.count(code_) == 1) {
    path_ = CODE_PATH.find(code_)->second;
    stat((src_dir_ + path_).data(), &file_stat_);
  }
}

void HTTPResponse::AddStateLine(Buffer &buffer) {
  std::string status;
  if (CODE_STATUS.count(code_) == 1) {
    status = CODE_STATUS.find(code_)->second;
  } else {
    code_ = 400;
    status = CODE_STATUS.find(400)->second;
  }
  buffer.Append("HTTP/1.1 " + std::to_string(code_) + " " + status + "\r\n");
}

void HTTPResponse::AddHeader(Buffer &buffer) {
  buffer.Append("Connection: ");
  if (is_keep_alive_) {
    buffer.Append("keep-alive\r\n");
    buffer.Append("keep-alive: max=6, timeout=120\r\n");
  } else {
    buffer.Append("close\r\n");
  }
  buffer.Append("Content-type: " + GetFileType() + "\r\n");
}

void HTTPResponse::AddContent(Buffer &buffer) {
  int srcFd = open((src_dir_ + path_).data(), O_RDONLY);
  if (srcFd < 0) {
    ErrorContent(buffer, "File NotFound!");
    return;
  }
  int *mmRet = (int *)mmap(nullptr, file_stat_.st_size, PROT_READ, MAP_PRIVATE,
                           srcFd, 0);
  if (*mmRet == -1) {
    ErrorContent(buffer, "File NotFound!");
    return;
  }
  file_ = (char *)mmRet;
  close(srcFd);
  buffer.Append("Content-length: " + std::to_string(file_stat_.st_size) +
                "\r\n\r\n");
}

std::string HTTPResponse::GetFileType() {
  std::string::size_type idx = path_.find_last_of('.');
  if (idx == std::string::npos) {
    return "text/plain";
  }
  std::string suffix = path_.substr(idx);
  if (SUFFIX_TYPE.count(suffix) == 1) {
    return SUFFIX_TYPE.find(suffix)->second;
  }
  return "text/plain";
}

void HTTPResponse::ErrorContent(Buffer &buff,
                                const std::string &message) const {
  std::string body;
  std::string status;
  body += "<html><title>Error</title>";
  body += "<body bgcolor=\"ffffff\">";
  if (CODE_STATUS.count(code_) == 1) {
    status = CODE_STATUS.find(code_)->second;
  } else {
    status = "Bad Request";
  }
  body += std::to_string(code_) + " : " + status + "\n";
  body += "<p>" + message + "</p>";
  body += "<hr><em>TinyWebServer</em></body></html>";
  buff.Append("Content-length: " + std::to_string(body.size()) + "\r\n\r\n");
  buff.Append(body);
}

char *HTTPResponse::File() { return file_; }

size_t HTTPResponse::FileLen() const { return file_stat_.st_size; }
