#include"http_conn.h"

int http_conn::m_epollfd = -1;
int http_conn::m_user_count = 0;
char* http_conn::srcDir = nullptr;

void setnonblocking(int fd)
{
  fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
}
//向EPOLL中添加监听的文件描述符
void addfd(int epollfd, int fd, bool one_shot)
{
  epoll_event event;
  event.data.fd = fd;
  event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
  if (one_shot)
  {
    //一个文件描述符引发的一个事件只能被一个线程处理
    event.events |= EPOLLONESHOT;
  }
  epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
  setnonblocking(fd);
}

void removefd(int epollfd, int fd)
{
  epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
  close(fd);
}

//重置EPOLL_ONSHOT,否则只触发一次
void modfd(int epollfd, int fd, int env)
{
  epoll_event event;
  event.data.fd = fd;
  event.events = env | EPOLLONESHOT | EPOLLET | EPOLLRDHUP;
  epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

//初始化连接
void http_conn::init(int sockfd, const sockaddr_in& addr)
{
  isClose = false;
  m_sockfd = sockfd;
  m_address = addr;
  // int reuse = 1;
  // setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
  //将这个连接socket加入epoll内核事件表
  addfd(m_epollfd, m_sockfd, true);
  m_user_count++;
  srcDir = getcwd(nullptr, 256);
  // 初始化资源路径
  strncat(srcDir, "/resource", 16);
  m_buffer.retriveAll();
  write_buffer.retriveAll();
  //初始化有限状态机的状态
  // init();
}

void http_conn::close_conn()
{
  if (m_sockfd != -1)
  {
    removefd(m_epollfd, m_sockfd);
    m_user_count--;
    m_sockfd = -1;
  }
}

//非阻塞的读取
ssize_t http_conn::read(int* saveErrno)
{
  //读取到的字节
  int bytes_read = -1;
  while (true)
  {
    //
    bytes_read = m_buffer.readfd(m_sockfd, saveErrno);
    if (bytes_read <= 0)
    {
      break;
    }
  }
  // test
  cout << "read successeds" << endl;
  m_buffer.print_test();
  return bytes_read;
}

ssize_t http_conn::write(int* saveErrno)
{
  ssize_t len = -1;
  ssize_t cnt = 0;
  cout << "begin write" << endl;
  while (true)
  {
    len = writev(m_sockfd, iov_, iovCnt_);
    cnt += len;
    // cout << iov_[0].iov_len << " " << iov_[1].iov_len << endl;
    // printf("iov:%x", iov_);s
    if (len < 0)
    {
      *saveErrno = errno;
      perror("writev");
      break;
    }
    if (iov_[0].iov_len + iov_[1].iov_len == 0)
    {
      cout << "write over" << endl;
      break;
    }
    else if (static_cast<size_t>(len) > iov_[0].iov_len) {
      iov_[1].iov_base = (uint8_t*)iov_[1].iov_base + (len - iov_[0].iov_len);
      iov_[1].iov_len -= (len - iov_[0].iov_len);
      // 重置缓冲区
      if (iov_[0].iov_len) {
        write_buffer.retriveAll();
        iov_[0].iov_len = 0;
      }
      cout << "write1 over" << endl;
    }
    else {
      iov_[0].iov_base = (uint8_t*)iov_[0].iov_base + len;
      iov_[0].iov_len -= len;
      write_buffer.retrieve(len);
      cout << "write0 over" << endl;
    }
  }
  cout << "write end and len:" << len << endl;
  return cnt;
}

//工作函数 解析HTTP请求，生成响应---注意这个函数是子线程执行的
void http_conn::process()
{
  // cout << "begin process" << endl;
  m_request.init();
  if (m_buffer.readable() <= 0)
    return;
  // cout << "parse begin" << endl;
  if (m_request.parse(m_buffer))
  {
    // cout << "parse end" << endl;
    // 初始化响应报文
    // cout << srcDir << m_request.URL() << endl;
    m_response.init(srcDir, m_request.URL(), true, 200);
    // cout << "init end" << endl;
  }
  else
  {
    // 初始化一个错误响应
    m_response.init(srcDir, m_request.URL(), false, 400);
  }
  // 将封装好的响应报文写入m_buffer
  m_response.makeresponse(write_buffer);
  iov_[0].iov_base = const_cast<char*>(write_buffer.begin_read());
  iov_[0].iov_len = write_buffer.readable();
  iovCnt_ = 1;
  // 如果有资源文件也会写入
  if (m_response.fileLen() > 0 && m_response.File())
  {
    // cout << "File ok" << endl;
    iov_[1].iov_base = m_response.File();
    iov_[1].iov_len = m_response.fileLen();
    iovCnt_ = 2;
  }
  modfd(m_epollfd, m_sockfd, EPOLLOUT);
}

void http_conn::process_afterWrite() {
  // int ret = -1;
  cout << "continue keepalive" << endl;
  if (m_request.isKeepAlive())
  {
    cout << "keepalive success" << endl;
    modfd(m_epollfd, m_sockfd, EPOLLIN);
    return;
  }
  else
  {
    cout << "keepalive fail" << endl;
    removefd(m_epollfd, m_sockfd);
    Close();
  }
}

void http_conn::Close() {
  m_response.unmapFile();
  if (isClose == false)
  {
    isClose = true;
    --m_user_count;
    close(m_sockfd);
  }
}

