#ifndef LOCKER_H
#define LOCKER_H
#include<pthread.h>
#include<exception>
#include<semaphore.h>
//线程同步机制封装类
//互斥锁类

class locker
{
public:
  locker()
  {
    if (pthread_mutex_init(&m_mutex, NULL) != 0)
    {
      throw std::exception();
    }
  }
  bool lock()
  {
    return pthread_mutex_lock(&m_mutex);
  }
  bool unlock()
  {
    return pthread_mutex_unlock(&m_mutex);
  }
  pthread_mutex_t* get()
  {
    return &m_mutex;
  }
  ~locker()
  {
    pthread_mutex_destroy(&m_mutex);
  }
private:
  pthread_mutex_t m_mutex;
};

class cond
{
public:
  cond()
  {
    if (pthread_cond_init(&m_cond, NULL) != 0)
    {
      throw std::exception();
    }
  }
  ~cond()
  {
    pthread_cond_destroy(&m_cond);
  }
  bool wait(locker& lock)
  {
    return pthread_cond_wait(&m_cond, lock.get()) == 0;
  }
  bool timedwait(locker& lock, struct timespec t)
  {
    return pthread_cond_timedwait(&m_cond, lock.get(), &t) == 0;
  }
  bool signal()
  {
    return pthread_cond_signal(&m_cond) == 0;
  }
  bool broadcase()
  {
    return pthread_cond_broadcast(&m_cond) == 0;
  }
private:
  pthread_cond_t m_cond;
};

class sem
{
public:
  sem()
  {
    if (sem_init(&m_sem, 0, 0) != 0)
    {
      throw std::exception();
    }
  }
  sem(int num)
  {
    if (sem_init(&m_sem, 0, num) != 0)
    {
      throw std::exception();
    }
  }
  ~sem()
  {
    sem_destroy(&m_sem);
  }
  bool wait()
  {
    return sem_wait(&m_sem) == 0;
  }
  //增加信号量
  bool post()
  {
    return sem_post(&m_sem) == 0;
  }

private:
  sem_t m_sem;
};
#endif
