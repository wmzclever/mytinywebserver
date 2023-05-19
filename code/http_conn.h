#ifndef HTTPCONN_H
#define HTTPCONN_H
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
#include"buffer.h"
#include"http_request.h"
#include"http_response.h"
//HTTP请求方法
enum METHOD { GET = 0, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, CONNECT };



//有限状态机的状态，行的读取状态
//完整的行，行出错，行数据不完整
enum LINE_STATUS { LINE_OK = 0, LINE_BAD, LINE_OPEN };

class http_conn {
public:
  static int m_epollfd;
  static int m_user_count;
  static const int READ_BUFFER_SIZE = 2048;
  static const int WRITE_BUFFER_SIZE = 1024;
  http_conn() = default;
  ~http_conn() {}
  //处理客户端请求并响应
  void process();
  void process_afterWrite();
  void init(int sockfd, const sockaddr_in& addr);
  void close_conn();
  ssize_t read(int* saveErrno);
  ssize_t write(int* saveErrno);
  void Close();
  // 资源路径
  static char* srcDir;
  // char* get_line() { return m_read_buf + m_start_line; }
// private:
  //socket通信描述符
  bool isClose = false;
  bool isET = false;
  int m_sockfd = 0;
  //客户端地址
  sockaddr_in m_address;
  //读缓冲 保存request报文
  Buffer m_buffer;
  // 写缓冲，用于保存respon报文
  Buffer write_buffer;
  // 请求报文
  http_request m_request;
  // 响应报文
  http_response m_response;

  int iovCnt_ = 0;

  struct iovec iov_[2] = { 0 };
};

#endif
