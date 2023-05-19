#include "http_request.h"
#include<regex>

const unordered_set<string> http_request::DEFAULT_HTML{
  "/index", "/register", "/login",
    "/welcome", "/video", "/picture", };

void http_request::init()
{
  method = url = version = body = "";
  state = REQUESTLINE;
  header.clear();
}

// 请求方法 url HTTP版本
bool http_request::parse_requestline(string& line) {
  regex pattern("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
  smatch subMatch;
  if (regex_match(line, subMatch, pattern))
  {
    method = subMatch[1];
    url = subMatch[2];
    version = subMatch[3];
    state = HEADER;
    // cout << method << " " << url << " " << version << " " << state << " " << endl;
    return true;
  }
  return false;
}

bool http_request::parse_header(string& line) {
  regex patten("^([^:]*): ?(.*)$");
  smatch subMatch;
  if (regex_match(line, subMatch, patten)) {
    // 存储header请求头
    header[subMatch[1]] = subMatch[2];
    return true;
  }
  else
    return false;
}

// 待完善，第一版只支持GET请求
bool http_request::parse_body(string& body)
{
  state = FINISH;
  return true;
}

bool http_request::parse(Buffer& m_buffer)
{
  const char CRLF[] = "\r\n";
  // have no data to read
  if (m_buffer.readable() <= 0)
    return false;
  // cout << "begin parse" << endl;
  while (m_buffer.readable() && state != FINISH)
  {
    char* lineEnd = search(m_buffer.begin_read(), m_buffer.begin_write(), CRLF, CRLF + 2);
    string line(m_buffer.begin_read(), lineEnd);
    // cout << "test:" << line << endl;
    // 有限状态机
    switch (state)
    {
    case REQUESTLINE:
      if (!parse_requestline(line))
        return false;
      parsePath();
      break;
    case HEADER:
      if (!parse_header(line) && m_buffer.readable() <= 2)
        state = FINISH;
      else if (!parse_header(line) && m_buffer.readable() > 2)
        state = BODY;
      // 没有请求body的报文
      // cout << "header ok" << endl;
      break;
    case BODY:
      if (!parse_body(line))
        return false;
      break;
    default:
      break;
    }
    // ***********
    if (lineEnd == m_buffer.begin_write())
      break;
    m_buffer.moverptr_until(lineEnd + 2);
  }

  return true;
}

void http_request::parsePath() {
  //若访问根目录则 返回默认文件
  if (url == "/") {
    url = "/index.html";
    // cout << "hh" << endl;
  }
  else {
    //匹配常见的资源，加上后缀名称
    for (auto& item : DEFAULT_HTML) {
      if (item == url) {
        url += ".html";
        break;
      }
    }
  }
}


bool http_request::isKeepAlive() {
  if (header.count("Connection") == 0)
    return true;
  else if (header.find("Connection")->second == "close")
    return false;
  else
    return false;
}

string http_request::URL() {
  return url;
}

string http_request::METHOD() {
  return method;
}

string http_request::VERSION() {
  return version;
}