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
#include "pthread_pool.h"
#include<cstring>
#include "http_conn.h"
#include<pthread.h>
#include<exception>
#include<semaphore.h>
#define MAX_FD 20
#define MAX_EVENT 10000
//添加信号捕捉
extern void addfd(int epollfd, int fd, bool one_shot);
extern void removefd(int epollfd, int fd);
extern void modfd(int epollfd, int fd, int env);
// class sem;

//添加信号捕捉
void addsig(int sig, void(*handler)(int))
{
  struct sigaction sa;
  bzero(&sa, sizeof(sa));
  sa.sa_handler = handler;
  // sa.sa_flags = 0;
  //运行期间屏蔽其他信号，反之被其他信号中断
  sigfillset(&sa.sa_mask);
  sigaction(sig, &sa, NULL);
}

int main(int argc, char* argv[])
{
  if (argc <= 1)
  {
    printf("argv is not enough\n");
    exit(-1);
  }
  int port = atoi(argv[1]);
  //忽略SIGPIPE信号，由于向一个读端关闭的socket或管道中写数据会触发SIGPIPE默认终止进程
  addsig(SIGPIPE, SIG_IGN);

  ThreadPool* pool = nullptr;
  // 如果有异常就立刻退出程序
  try
  {
    pool = new ThreadPool();
  }
  catch (...) {
    exit(-1);
  }
  //保存所有客户端信息
  http_conn* users = new http_conn[MAX_FD];
  int listenfd = socket(PF_INET, SOCK_STREAM, 0);

  //端口复用,注意该行位置，在socket创建之后
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
  epoll_event events[MAX_EVENT];
  int epollfd = epoll_create(5);
  addfd(epollfd, listenfd, false);
  http_conn::m_epollfd = epollfd;
  http_conn::m_user_count = 0;
  while (true)
  {
    int num = epoll_wait(epollfd, events, MAX_EVENT, -1);
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
        struct sockaddr_in client_address;
        socklen_t client_addrlen = sizeof(client_address);
        int connfd = accept(listenfd, (struct sockaddr*)&client_address, &client_addrlen);
        if (http_conn::m_user_count >= MAX_FD)
        {
          close(connfd);
          continue;
        }
        users[connfd].init(connfd, client_address);
        // cout << "listen successed" << endl;
        cout << "connfd:" << connfd << endl;
      }
      //事件出错，关闭链接
      else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
      {
        users[sockfd].close_conn();
      }
      //读事件
      else if (events[i].events & EPOLLIN) {
        // 模拟proactor模式，主线程负责读写数据，子线程负责处理逻辑
        // cout << "read wait" << endl;c
        if (users[sockfd].read(&errno)) {
          // printf("user addr1:%x\n", users[sockfd]);
          // pool->append(bind(&http_conn::process, users[sockfd]));
          pool->append([&users, sockfd] {users[sockfd].process();});
          // cout << "main test printf:\n";
          // users[sockfd].write_buffer.print_test();
          // getchar();
        }
        else {
          users[sockfd].close_conn();
        }
      }
      //写事件
      else if (events[i].events & EPOLLOUT)
      {
        // cout << "main test printf:\n";
        users[sockfd].write_buffer.print_test();
        // getchar();
        if (users[sockfd].write(&errno))
        {
          // cout << "write end in main" << endl;
          // pool->append(bind(&http_conn::process_afterWrite, users[sockfd]));
          pool->append([&users, sockfd] {users[sockfd].process_afterWrite();});
        }
        else {
          // cout << "write fail in main" << endl;
          users[sockfd].close_conn();
        }
      }
    }
  }
  close(epollfd);
  close(listenfd);
  delete[] users;
  delete[] pool;
  return 0;
}


