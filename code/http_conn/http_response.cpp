#include"./http_response.h"

const unordered_map<string, string> http_response::SUFFIX_TYPE = {
    { ".html",  "text/html" },
    { ".xml",   "text/xml" },
    { ".xhtml", "application/xhtml+xml" },
    { ".txt",   "text/plain" },
    { ".rtf",   "application/rtf" },
    { ".pdf",   "application/pdf" },
    { ".word",  "application/nsword" },
    { ".png",   "image/png" },
    { ".gif",   "image/gif" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },
    { ".au",    "audio/basic" },
    { ".mpeg",  "video/mpeg" },
    { ".mpg",   "video/mpeg" },
    { ".avi",   "video/x-msvideo" },
    { ".gz",    "application/x-gzip" },
    { ".tar",   "application/x-tar" },
    { ".css",   "text/css "},
    { ".js",    "text/javascript "},
};

const unordered_map<int, string> http_response::CODE_STATUS = {
    { 200, "OK" },
    { 400, "Bad Request" },
    { 403, "Forbidden" },
    { 404, "Not Found" },
};

const unordered_map<int, string> http_response::CODE_PATH = {
    { 400, "/400.html" },
    { 403, "/403.html" },
    { 404, "/404.html" },
};

http_response::http_response()
{
  code = -1;
  path = srcDir = "";
  isKeepAlive = false;
  mmFile = nullptr;
  mmFileStat = { 0 };
  // cout << "constructed end" << endl;
}

http_response::~http_response()
{
  munmap(mmFile, mmFileStat.st_size);
  mmFile = nullptr;
}


// srcDir是资源目录 path是资源名称
void http_response::init(const string& srcDir, string path, bool isKeepAlive, int code) {
  // cout << "init begin" << endl;
  if (mmFile)
  {
    munmap(mmFile, mmFileStat.st_size);
    mmFile = nullptr;
  }
  this->code = code;
  this->isKeepAlive = isKeepAlive;
  this->path = path;
  this->srcDir = srcDir;
  this->mmFile = nullptr;
  this->mmFileStat = { 0 };
}

void http_response::makeresponse(Buffer& m_buffer) {
  // 资源找不到
  if (stat((srcDir + path).data(), &mmFileStat) < 0 || S_ISDIR(mmFileStat.st_mode))
  {
    code = 404;
    path = CODE_PATH.find(code)->second;
    stat((srcDir + path).data(), &mmFileStat);
  }
  // 服务器禁止访问，没有访问权限
  else if (!(mmFileStat.st_mode & S_IROTH))
  {
    code = 403;
    path = CODE_PATH.find(code)->second;
  }
  // 可以访问
  else if (code == -1)
  {
    code = 200;
  }
  addstateLine(m_buffer);
  addheader(m_buffer);
  addcontent(m_buffer);
}

// 请求首行
void http_response::addstateLine(Buffer& m_buffer) {
  string status;
  unordered_map<int, string>::const_iterator iter;
  if ((iter = CODE_STATUS.find(code)) != CODE_STATUS.end())
  {
    status = iter->second;
  }
  else {
    code = 400;
    status = CODE_STATUS.find(400)->second;
    path = CODE_PATH.find(code)->second;
  }
  m_buffer.append("HTTP/1.1 " + to_string(code) + " " + status + "\r\n");
}

// 请求头
void http_response::addheader(Buffer& m_buffer) {
  m_buffer.append("Connection: ");
  if (isKeepAlive)
  {
    m_buffer.append("keep-alive\r\n");
    m_buffer.append("keep-alive: max=6, timeout=120\r\n");
  }
  else {
    m_buffer.append("close\r\n");
  }
  m_buffer.append("Content-type: " + getfiletype() + "\r\n");
}

// 请求体
void http_response::addcontent(Buffer& m_buffer) {
  int fd = open((srcDir + path).data(), O_RDONLY);
  if (fd < 0)
  {
    errorContent(m_buffer, "FILE NOT FOUND!");
    return;
  }
  void* mmret = mmap(0, mmFileStat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (mmret == MAP_FAILED)
  {
    perror("mmap");
    mmret = nullptr;
    errorContent(m_buffer, "FILE NOT FOUND!");
    return;
  }
  mmFile = (char*)mmret;
  close(fd);
  m_buffer.append("Content-length: " + to_string(mmFileStat.st_size) + "\r\n\r\n");
}

string http_response::getfiletype()
{
  string::size_type idx = path.find_last_of('.');
  if (idx == string::npos)
  {
    return "text/plain";
  }
  string suffix = path.substr(idx);
  if (SUFFIX_TYPE.count(suffix) == 1)
  {
    return SUFFIX_TYPE.find(suffix)->second;
  }
  return "text/plain";
}

void http_response::errorContent(Buffer& m_buffer, string message)
{
  string body;
  string status;
  body += "<html><title>Error</title>";
  body += "<body bgcolor=\"ffffff\">";
  if (CODE_STATUS.count(code) == 1) {
    status = CODE_STATUS.find(code)->second;
  }
  else {
    status = "Bad Request";
  }
  body += to_string(code) + " : " + status + "\n";
  body += "<p>" + message + "</p>";
  body += "<hr><em>Wmz-WebServer-version1.0</em></body></html>";

  m_buffer.append("Content-length: " + to_string(body.size()) + "\r\n\r\n");
  m_buffer.append(body);
}


char* http_response::File() {
  return mmFile;
}


size_t  http_response::fileLen() {
  return mmFileStat.st_size;
}


void  http_response::unmapFile() {
  if (mmFile)
  {
    munmap(mmFile, mmFileStat.st_size);
    mmFile = nullptr;
  }
}