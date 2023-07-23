
#ifndef MY_TIMER_H
#define MY_TIMER_H

#include <queue>
#include <unordered_map>
#include <time.h>
#include <algorithm>
#include <arpa/inet.h> 
#include <functional> 
#include <assert.h> 
#include <chrono>

using namespace std;
// 定义回调函数
using TimeCallBack = function<void()>;
// 使用高精度时钟类型
using Clock = chrono::high_resolution_clock;
// 使用毫秒来作为时间单位
using Ms = chrono::milliseconds;
// 时间点类型
using TimeStamp = Clock::time_point;

// 堆中的一个节点类型
struct TimeNode
{
  //连接socket描述符
  int fd;
  // 过期时间
  TimeStamp expires;
  // 过期回调函数
  TimeCallBack func;
  bool operator<(const TimeNode& t)
  {
    return expires < t.expires;
  }
};


class HeapTimer
{
public:
  HeapTimer() {
    heap.reserve(64);
  }
  ~HeapTimer() { clear(); }
  void adjust(int fd, int newexpires);
  void add(int fd, int time, const TimeCallBack& func);
  void tick();
  void pop();
  int getNextTick();
  void clear();
private:
  void del(size_t i);
  void siftup(size_t i);
  bool siftdown(size_t i, size_t n);
  void swap(size_t i, size_t j);
  // 最小堆数组
  vector<TimeNode> heap;
  // 文件描述符和堆中索引的映射
  unordered_map<int, size_t> mmap;

};



#endif








