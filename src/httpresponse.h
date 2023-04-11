#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include "buffer.h"
#include <fcntl.h>    // open
#include <sys/mman.h> // mmap, munmap
#include <sys/stat.h> // stat
#include <unistd.h>   // close
#include <unordered_map>

class HTTPResponse {
public:
  HTTPResponse() = default;
  ~HTTPResponse() { UnmapFile(); };

  void Init(const std::string &srcDir, std::string &path,
            bool isKeepAlive = false, int code = -1);
  void MakeResponse(Buffer &buffer);
  void UnmapFile() { munmap(file_, file_stat_.st_size); };
  char *File();
  size_t FileLen() const;
  void ErrorContent(Buffer &buff, const std::string &message) const;
  int Code() const { return code_; }

private:
  void AddStateLine(Buffer &buff);
  void AddHeader(Buffer &buff);
  void AddContent(Buffer &buff);
  void ErrorHtml();
  std::string GetFileType();

  int code_{-1};
  bool is_keep_alive_{false};
  std::string path_;
  std::string src_dir_;
  char *file_{nullptr};
  struct stat file_stat_ {
    0
  };
  static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;
  static const std::unordered_map<int, std::string> CODE_STATUS;
  static const std::unordered_map<int, std::string> CODE_PATH;
};

#endif // HTTP_RESPONSE_H