#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#ifndef WIN32
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#endif
#include <string.h>

#include <algorithm>

#include "RunLoop.h"

ReadReady::~ReadReady()
{
}

WriteReady::~WriteReady()
{
}

RunLoop::RunLoop()
{
  m_stop = false;
}

void RunLoop::registerReadIntent(int fd, ReadReady* readReady)
{
  readIntents[fd] = readReady;
}

void RunLoop::registerWriteIntent(int fd, WriteReady* writeReady)
{
  writeIntents[fd] = writeReady;
}

void RunLoop::run()
{
  do
  {
    fd_set readset;
    fd_set writeset;
    FD_ZERO(&readset);
    FD_ZERO(&writeset);

    for ( std::map<int, ReadReady*>::const_iterator pos = readIntents.begin(); pos != readIntents.end(); ++pos )
      FD_SET(pos->first, &readset);

    for ( std::map<int, WriteReady*>::const_iterator pos = writeIntents.begin(); pos != writeIntents.end(); ++pos )
      FD_SET(pos->first, &writeset);

    int maxRead = readIntents.empty() ? 0 : readIntents.rbegin()->first;
    int maxWrite = writeIntents.empty() ? 0 : writeIntents.rbegin()->first;

    int result = 0;

    result = select(std::max(maxRead, maxWrite) + 1, &readset, &writeset, NULL, NULL);
    if ( result == -1 && errno == EINTR )
      continue;

    if ( result < 0 )
    {
      fprintf(stderr, "Error on select(): %s\n", strerror(errno));
      break;
    }

    std::map<int, ReadReady*> readIntentsCopy = readIntents;
    std::map<int, WriteReady*> writeIntentsCopy = writeIntents;
    readIntents.clear();
    writeIntents.clear();
    for ( std::map<int, ReadReady*>::const_iterator pos = readIntentsCopy.begin(); pos != readIntentsCopy.end(); ++pos )
    {
      if ( FD_ISSET(pos->first, &readset) )
      {
        pos->second->doRead(this, pos->first);
      }
    }

    for ( std::map<int, WriteReady*>::const_iterator pos = writeIntentsCopy.begin(); pos != writeIntentsCopy.end(); ++pos )
    {
      if ( FD_ISSET(pos->first, &writeset) )
      {
        pos->second->doWrite(this, pos->first);
      }
    }
    FD_ZERO(&readset);
    FD_ZERO(&writeset);
  }
  while ( ! m_stop );
}

void RunLoop::stop()
{
  m_stop = true;
}

