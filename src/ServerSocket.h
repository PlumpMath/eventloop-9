#ifndef SERVERSOCKET_H
#define SERVERSOCKET_H

#include "Net.h"

#define SOCKET_NOT_CONNECTED (-1)

class ServerSocket
{
public:
  ServerSocket();
  void connect(const char* addr, int port, bool nonblock);
  SocketType getSocket() const;
  ~ServerSocket();
private:
  void throwAndCloseSocket(const char* err);

  SocketType m_socket;
};

#endif

