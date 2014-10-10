#ifndef __RUNLOOP_H__
#define __RUNLOOP_H__

#include <map>

class RunLoop;

class ReadReady
{
public:
  virtual void doRead(RunLoop* loop, int fd) = 0;
  virtual ~ReadReady();
};

class WriteReady
{
public:
  virtual void doWrite(RunLoop* loop, int fd) = 0;
  virtual ~WriteReady();
};

class RunLoop
{
public:
  RunLoop();
  void registerReadIntent(int fd, ReadReady* readReady);
  void registerWriteIntent(int fd, WriteReady* writeReady);
  void run();
  void stop();

private:
  bool m_stop;
  std::map<int, ReadReady*> readIntents;
  std::map<int, WriteReady*> writeIntents;
};

#endif

