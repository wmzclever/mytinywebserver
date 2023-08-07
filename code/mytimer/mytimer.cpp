#include"./mytimer.h"

// 设置连接socket的新过期时间
void HeapTimer::adjust(int fd, int newexpires) {
  heap[mmap[fd]].expires = Clock::now() + Ms(newexpires);
  siftdown(mmap[fd], heap.size());
}

// 将新的socket描述符加入最小堆
void HeapTimer::add(int fd, int time, const TimeCallBack& func) {
  size_t i;
  if (mmap.count(fd) == 0)
  {
    i = heap.size();
    mmap[fd] = i;
    heap.push_back({ fd,Clock::now() + Ms(time),func });
    siftup(i);
  }
  else {
    i = mmap[fd];
    heap[i].expires = Clock::now() + Ms(time);
    heap[i].func = func;
    if (!siftdown(i, heap.size()))
      siftup(i);
  }
}

// 将堆顶节点拿出，并调用其回调函数
void HeapTimer::tick() {
  if (heap.empty())
    return;
  while (!heap.empty())
  {
    TimeNode node = heap.front();
    if (chrono::duration_cast<Ms>(node.expires - Clock::now()).count() > 0)
      break;
    // 超时就调用回调函数
    node.func();
    pop();
    // std::cout << "heap size:" << heap.size() << std::endl;
  }
}

void HeapTimer::pop() {
  del(0);
}

// 将堆顶节点弹出并返回下一个最小过期时间
int HeapTimer::getNextTick() {
  tick();
  size_t res = -1;
  if (!heap.empty())
  {
    res = chrono::duration_cast<Ms>(heap.front().expires - Clock::now()).count();
    if (res < 0) res = 0;
  }
  return res;
}

void HeapTimer::clear() {
  heap.clear();
  mmap.clear();
}


// 删除索引为index的堆节点
void HeapTimer::del(size_t index) {
  size_t i = index;
  size_t n = heap.size() - 1;
  if (i < n)
  {
    swap(i, n);
    // 如果不是删除的头节点 可能会发生
    if (!siftdown(i, n))
      siftup(i);
  }
  mmap.erase(heap.back().fd);
  heap.pop_back();
}

// 向上调整堆，每次添加到最后一个位置上，向上调整；
void HeapTimer::siftup(size_t i) {
  size_t j = (i - 1) / 2;
  while (j >= 0)
  {
    if (heap[j] < heap[i])
      break;
    swap(i, j);
    i = j;
    j = (i - 1) / 2;
  }
}

// 向下调整堆
bool HeapTimer::siftdown(size_t index, size_t n) {
  size_t i = index;
  size_t j = i * 2 + 1;
  while (j < n)
  {
    // j指向的是两个孩子中最小的
    if (j + 1 < n && heap[j + 1] < heap[j])++j;
    if (heap[i] < heap[j]) break;
    swap(i, j);
    i = j;
    j = i * 2 + 1;
  }
  return i > index;
}

void HeapTimer::swap(size_t i, size_t j) {
  std::swap(heap[i], heap[j]);
  mmap[heap[i].fd] = i;
  mmap[heap[j].fd] = j;
}