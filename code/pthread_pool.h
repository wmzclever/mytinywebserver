#ifndef PTHREAD_POOL_H
#define PTHREAD_POOL_H

#include <cstdio>
#include <stdlib.h>
#include <pthread.h>
#include <list>
#include "locker.h"
#include <functional>
#include <queue>
using namespace std;
//线程池类，模板类为了代码复用,T表示任务函数
class ThreadPool
{
public:
  ThreadPool(int thread_number = 8, int max_requests = 1000);
  ~ThreadPool();
  template<typename T>
  bool append(T&& work);
  void run();
private:
  //线程池工作线程数量
  int m_thread_number;
  //线程数组
  pthread_t* m_threads;
  //最大请求数
  int m_max_request;
  //工作队列
  queue<function<void()>> m_workqueue;
  //互斥访问工作队列
  locker m_queuemutex;
  //信号量表示队列中任务数量
  sem m_queuestat;
  //是否关闭
  bool isclose;
  static void* worker(void* arg);
};

ThreadPool::ThreadPool(int thread_number, int max_requests) :m_thread_number(thread_number), m_max_request(max_requests), isclose(false), m_threads(nullptr)
{
  if ((thread_number <= 0) || (max_requests <= 0))
    throw std::exception();
  m_threads = new pthread_t[m_thread_number];
  if (!m_threads)
  {
    throw std::exception();
  }
  for (int i = 0;i < thread_number;++i)
  {
    printf("create the %dth thread\n", i);
    //创建线程
    if (pthread_create(m_threads + i, NULL, worker, static_cast<void*>(this)) != 0)
    {
      delete[] m_threads;
      throw std::exception();
    }
    //设置线程分离，返回0成功，非0失败
    if (pthread_detach(m_threads[i]))
    {
      delete[] m_threads;
      throw std::exception();
    }
  }
}

ThreadPool::~ThreadPool()
{
  delete[] m_threads;
  isclose = true;
}

// 采用了万能引用，由于引用折叠，这样做能保留原有的类型
template<typename T>
bool ThreadPool::append(T&& request)
{
  m_queuemutex.lock();
  // 完美转发，保留原有的类型，否则即使request是右值，但是这个变量名仍是左值；
  m_workqueue.push(forward<T>(request));
  m_queuemutex.unlock();
  m_queuestat.post();
  return true;
}

void ThreadPool::run()
{
  while (!isclose)
  {
    m_queuestat.wait();
    m_queuemutex.lock();
    auto request = m_workqueue.front();
    m_workqueue.pop();
    m_queuemutex.unlock();
    if (!request)
    {
      continue;
    }
    //处理工作
    request();
  }
}

//线程执行的任务
void* ThreadPool::worker(void* arg)
{
  //个工作线程
  ThreadPool* pool = static_cast<ThreadPool*>(arg);
  pool->run();
  return pool;
}

#endif

