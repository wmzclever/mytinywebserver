#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H
#include<iostream>
#include<cstdio>
#include<cstdlib>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<errno.h>
#include<fcntl.h>
#include<sys/epoll.h>
#include<signal.h>
#include<cstring>
#include<string>
#include<unordered_map>
#include<algorithm>
#include<sys/mman.h>
#include <sys/stat.h>    // stat
#include"../buffer/buffer.h"
using namespace std;

class http_response
{
public:
  http_response();
  ~http_response();

  void init(const string& srcDir, string path, bool isKeepAlive, int code);
  void makeresponse(Buffer& m_buffer);
  char* File();
  size_t fileLen();
  void unmapFile();
private:
  void addstateLine(Buffer& m_buffer);
  void addheader(Buffer& m_buffer);
  void addcontent(Buffer& m_buffer);
  string getfiletype();
  void errorContent(Buffer& m_buffer, string message);

  // 状态码
  int code;
  // 是否处于连接状态
  bool isKeepAlive;
  // 资源路径
  string path;
  // 资源目录
  string srcDir;
  // 文件指针，指向内存映射的地址
  char* mmFile;
  // 文件属性信息
  struct stat mmFileStat;
  // 文件格式与MIME格式的映射
  static const unordered_map<string, string> SUFFIX_TYPE;
  // 状态码和状态信息的映射
  static const unordered_map<int, string> CODE_STATUS;
  // 错误状态码和错误资源路径的映射
  static const unordered_map<int, string> CODE_PATH;
};


#endif

