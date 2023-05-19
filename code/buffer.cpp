#include"buffer.h"

Buffer::Buffer(int bufsize) :readPos(0), writePos(0), bufsize(bufsize), m_buffer(bufsize, '\0') {}

//buff可读字节大小
size_t Buffer::readable() const {
  return writePos - readPos;
}

//buff可写字节大小
size_t Buffer::writeable() const {
  return bufsize - writePos;
}

//buff可以开始读的位置指针
char* Buffer::begin_read() {
  return &m_buffer[readPos];
}
//buff可以开始写的位置指针
char* Buffer::begin_write() {
  return &m_buffer[writePos];
}

//从socket中读数据到buff缓冲(写入buff)
size_t Buffer::readfd(int fd, int* saveerrno) {
  struct iovec iov[2];
  char buf[65535];
  iov[0].iov_base = begin_write();
  iov[0].iov_len = writeable();
  iov[1].iov_base = buf;
  iov[1].iov_len = strlen(buf);
  // cout << "begin read" << endl;
  ssize_t len = readv(fd, iov, 2);
  // cout << "read len" << len << endl;
  int Writeable = writeable();
  if (len < 0)
  {
    perror("Buffer::readfd");
    *saveerrno = errno;
  }
  else if (len <= Writeable)
  {
    writePos += len;
  }
  else {
    writePos = m_buffer.size();
    makespace(len - Writeable);
    // copy(buf, buf + len - Writeable, begin_write());
    // writePos += len - Writeable;
    append(buf, len - Writeable);
  }
  return len;
}

//向socket写数据，将buff中可读的内容写出
size_t Buffer::writefd(int fd, int* saveerrno) {
  size_t readsize = readable();
  size_t len = write(fd, begin_read(), readsize);
  if (len < 0)
  {
    *saveerrno = errno;
    return len;
  }
  readPos += len;
  return len;
}

//向buff中写数据
void Buffer::append(const char* str, size_t len)
{
  if (!str)
    return;
  if (writeable() < len)
    makespace(len);
  copy(str, str + len, begin_write());
  writePos += len;
}

void Buffer::append(const string& str)
{
  append(str.data(), str.size());
}

//将buff紧凑或扩容
void Buffer::makespace(size_t len) {
  //如果可以通过紧凑获得空间，则不必扩容；
  if (readPos + writeable() >= len)
  {
    copy(&m_buffer[0] + readPos, &m_buffer[0] + writePos, &m_buffer[0]);
    readPos = 0;
    writePos = readPos + readable();
  }
  //扩容
  else {
    m_buffer.resize(writePos + len + 1);
    bufsize = m_buffer.size();
  }
}

// 移动读指针到end
void Buffer::moverptr_until(const char* end)
{
  // assert(end >= begin_read());
  readPos += end - begin_read();

}

// 复位缓冲区
void Buffer::retriveAll() {
  bzero(&m_buffer[0], m_buffer.size());
  readPos = 0;
  writePos = 0;
}

//移动读指针，表示读出了len个数据
void Buffer::retrieve(size_t len)
{
  readPos += len;
}

void  Buffer::print_test()
{
  auto iter = begin_read();
  while (iter != begin_write())
  {
    cout << *iter;
    ++iter;
  }
  cout << endl;
}







