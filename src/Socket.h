#ifndef SOCKET_H
#define SOCKET_H

#include <string>

#include "Net.h"

#define SOCKET_NOT_CONNECTED (-1)

class Socket
{
public:
  Socket();
  void connect(const char* addr, int port, bool nonblock);
  SocketType getSocket() const;
  ~Socket();
private:
  void throwAndCloseSocket(const char* err);

  SocketType m_socket;
};

class ServerSocket
{
public:
  ServerSocket();
  void listen(const char* bindingAddr, int port, int backlog);
  void accept(std::string* ip, int* port);
  SocketType getClientSocket() const;
  ~ServerSocket();
private:
  SocketType m_serverSocket;
  SocketType m_clientSocket;
};

#endif

