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
#include<pthread.h>
#include<exception>
#include<semaphore.h>
#include "./http_conn/http_conn.h"
#include "./pool/pthread_pool.h"
#include"./mytimer/mytimer.h"
#include <memory>

using namespace std;
// 最大socket连接数量
#define MAX_FD 30000
// 最大的事件数量
#define MAX_EVENT 30000
//向epoll队列增删改信号捕捉
extern void addfd(int epollfd, int fd, bool one_shot);
extern void removefd(int epollfd, int fd);
extern void modfd(int epollfd, int fd, int env);


//添加信号捕捉
void addsig(int sig, void(*handler)(int))
{
  // 初始化信号结构体
  struct sigaction sa;
  bzero(&sa, sizeof(sa));
  sa.sa_handler = handler;
  // sa.sa_flags = 0;
  //运行期间屏蔽其他信号，防止被其他信号中断
  sigfillset(&sa.sa_mask);
  // 注册信号
  sigaction(sig, &sa, NULL);
}
// 关闭连接的回调函数
void closeConn(http_conn* client)
{
  if (!client)
    return;
  client->Close();
  // cout << "timesout close ok" << endl;
}


int main(int argc, char* argv[])
{
  if (argc <= 1)
  {
    printf("argv is not enough\n");
    exit(-1);
  }
  // 服务器端 端口号
  int port = atoi(argv[1]);
  //忽略SIGPIPE信号，由于向一个读端关闭的socket或管道中写数据会触发SIGPIPE默认终止进程
  addsig(SIGPIPE, SIG_IGN);

  ThreadPool* pool = nullptr;
  // 初始化堆定时器
  unique_ptr<HeapTimer> timer(new HeapTimer);
  // 设置默认超时时间
  int timeOutMs = 50000;
  // 如果有异常就立刻退出程序
  try
  {
    pool = new ThreadPool(20, 2000);
  }
  catch (...) {
    exit(-1);
  }
  //保存所有客户端信息
  http_conn* users = new http_conn[MAX_FD];

  int listenfd = socket(PF_INET, SOCK_STREAM, 0);

  //开启端口复用,注意该行位置，在socket创建之后
  int reuse = 1;
  setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

  //绑定
  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_port = htons(port);
  address.sin_addr.s_addr = INADDR_ANY;

  int ret = bind(listenfd, (struct sockaddr*)(&address), sizeof(address));
  if (ret < 0)
  {
    perror("bind");
    exit(1);
  }
  ret = listen(listenfd, 5);
  if (ret < 0)
  {
    perror("listen");
    exit(1);
  }

  // 使用epoll监听事件
  epoll_event events[MAX_EVENT];
  int epollfd = epoll_create(512);
  addfd(epollfd, listenfd, false);

  // struct epoll_event ev = { 0 };
  // ev.data.fd = listenfd;
  // ev.events = EPOLLIN;
  // epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev);

  http_conn::m_epollfd = epollfd;
  http_conn::m_user_count = 0;
  while (true)
  {
    int timeMs = -1;
    if (timeOutMs > 0)
    {
      // 删除堆顶超时连接，并获得下一个最小的超时时间，没有连接则返回-1
      timeMs = timer->getNextTick();
    }
    // !!!等待超时时间过了就之间返回0，停止阻塞；若设置为-1就会一直阻塞在epoll_wait等待事件发生
    //这样做可以及时处理超时事件，将其移除最小堆并调用回调函数关闭连接；
    // 一个小重点
    int num = epoll_wait(epollfd, events, MAX_EVENT, timeMs);
    // cout << "wait end" << endl;
    if (num < 0 && errno != EINTR)
    {
      printf("epoll failure\n");
      break;
    }
    for (int i = 0;i < num;++i)
    {
      int sockfd = events[i].data.fd;
      if (sockfd == listenfd)
      {
        do {
          // cout << "come in" << endl;
          struct sockaddr_in client_address;
          socklen_t client_addrlen = sizeof(client_address);
          int connfd = accept(listenfd, (struct sockaddr*)&client_address, &client_addrlen);
          if (connfd <= 0) {
            // cout << "while out" << endl;
            break;
          }
          if (http_conn::m_user_count >= MAX_FD)
          {
            cerr << "连接数超过限制" << endl;
            close(connfd);
            break;
          }
          users[connfd].init(connfd, client_address, timeOutMs);
          // 将新连接的socket加入最小堆定时器
          timer->add(connfd, timeOutMs, [&users, connfd] {closeConn(&users[connfd]);});
        } while (true);
        // cout << "listen successed" << endl;
// cout << "connfd:" << connfd << endl;
      }
      //事件出错，关闭链接
      else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
      {
        users[sockfd].close_conn();
      }
      //读事件
      else if (events[i].events & EPOLLIN) {
        // 模拟proactor模式，主线程负责读写数据，子线程负责处理逻辑
        // cout << "read wait" << endl;
        // 重置定时器
        timer->adjust(sockfd, timeOutMs);
        // int readNum = 0;
        // if ((readNum = users[sockfd].read(&errno)) > 0) {
          // printf("user addr1:%x\n", users[sockfd]);
          // pool->append(bind(&http_conn::process, users[sockfd]));
        pool->append([&users, sockfd] {users[sockfd].process();});
        // cout << "main test printf:\n";
        // users[sockfd].write_buffer.print_test();
        // getchar();
        // cout << "读事件成功" << endl;
      // }
      // else {
      //   users[sockfd].Close();
      // }
      }
      //写事件
      else if (events[i].events & EPOLLOUT)
      {
        // cout << "main test printf:\n";
        timer->adjust(sockfd, timeOutMs);
        // users[sockfd].write_buffer.print_test();
        // getchar();
        // int writeNum = 0;
        // if ((writeNum = users[sockfd].write(&errno)) > 0)
        // {
          // cout << "write end in main" << endl;
          // pool->append(bind(&http_conn::process_afterWrite, users[sockfd]));
        pool->append([&users, sockfd] {users[sockfd].process_afterWrite();});
        // cout << "写事件成功" << endl;
      // }
      // else {
      //   // cout << "write fail in main" << endl;
      //   users[sockfd].close_conn();
      // }
      }
    }
  }
  close(epollfd);
  close(listenfd);
  delete[] users;
  delete[] pool;
  return 0;
}


