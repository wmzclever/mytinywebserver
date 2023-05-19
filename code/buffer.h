#ifndef BUFFER_H
#define BUFFER_H

#include<vector>
#include<string>
#include <sys/uio.h> //readv
#include <unistd.h>  // write
#include <iostream>
#include <cstring>
#include<algorithm>
using namespace std;

class Buffer
{
public:
  Buffer(int bufsize = 1024);
  ~Buffer() = default;
  size_t readable() const;
  size_t writeable() const;
  char* begin_read();
  char* begin_write();
  size_t readfd(int fd, int* saveerrno);
  size_t writefd(int fd, int* saveerrno);
  void retriveAll();
  void moverptr_until(const char* end);
  void append(const char* str, size_t len);
  void append(const string& str);
  void makespace(size_t len);
  void retrieve(size_t len);
  void print_test();
private:
  vector<char> m_buffer;
  size_t readPos;
  size_t writePos;
  int bufsize;
};









#endif