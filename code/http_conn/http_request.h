#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H
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
#include<unordered_set>
#include<algorithm>
#include"../buffer/buffer.h"
using namespace std;

class http_request
{
public:
  //主状态机状态
  enum CHECK_STATE { REQUESTLINE = 0, HEADER, BODY, FINISH };

  //服务器处理HTTP请求的结果，报文解析的结果
  enum HTTP_CODE { NO_REQUEST, GET_REQUEST, BAD_REQUEST, NO_RESOURCE, FORBIDDEN_REQUEST, FILE_REQUEST };

  http_request() { init(); }
  ~http_request() = default;
  void init();
  bool parse(Buffer& m_buffer);

  bool parse_requestline(string& line);
  bool parse_header(string& header);
  bool parse_body(string& body);
  string URL();
  string METHOD();
  string VERSION();
  bool isKeepAlive();
  void parsePath();
  static const unordered_set<string> DEFAULT_HTML;
private:
  CHECK_STATE state;
  string method, url, version; //请求行
  unordered_map<string, string> header;

  // unordered_map<string, string> body;
  string body; //请求体
};




#endif