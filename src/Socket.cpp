#include "Socket.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdexcept>

static void ReuseAddr(int fd) 
{
  int yes = 1;
  if ( setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1 ) 
    throw std::runtime_error("getaddrinfo");
}

static void NonBlock(int fd)
{
    int flags;

    if ((flags = fcntl(fd, F_GETFL)) == -1) 
      throw std::runtime_error("fcntl");

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) 
      throw std::runtime_error("fcntl");
}

Socket::Socket()
{
  m_socket = SOCKET_NOT_CONNECTED;
}

struct FreeAddrInfo
{
  addrinfo** m_info;
  FreeAddrInfo(addrinfo** info)
  {
    m_info = info;
  }

  ~FreeAddrInfo()
  {
    if ( *m_info ) 
      freeaddrinfo(*m_info);
  }
};

struct CloseSocketIfError
{
  SocketType* m_socket;
  bool m_enable;
  CloseSocketIfError(SocketType* socket)
    : m_socket(socket), m_enable(true)
  {
  }
  void disable()
  {
    m_enable = false;
  }
  ~CloseSocketIfError()
  {
    if ( ! m_enable )
      return;
    if ( m_socket && *m_socket != SOCKET_NOT_CONNECTED )
    {
      close(*m_socket);
      *m_socket = SOCKET_NOT_CONNECTED;
    }
  }
};

static void Listen(SocketType s, sockaddr* sa, socklen_t len, int backlog) 
{
  CloseSocketIfError closeSocketIfError(&s);
  if ( ::bind(s, sa, len) == -1 ) 
    throw std::runtime_error("bind");

  if ( ::listen(s, backlog) == -1)
    throw std::runtime_error("listen");

  closeSocketIfError.disable();
}

void Socket::connect(const char* addr, int port, bool nonblock)
{
  int rv;
  char portstr[100];
  struct addrinfo hints, *servinfo, *bservinfo, *p, *b;

  servinfo = NULL;
  FreeAddrInfo freeAddrInfo(&servinfo);
  CloseSocketIfError closeSocketIfError(&m_socket);

  sprintf(portstr, "%d", port);
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  if ( ( rv = getaddrinfo(addr, portstr, &hints, &servinfo)) != 0 ) 
    throw std::runtime_error("getaddrinfo");

  for ( p = servinfo; p != NULL; p = p->ai_next ) 
  {
      if ( (m_socket = socket(p->ai_family,p->ai_socktype, p->ai_protocol) ) == -1)
          continue;
      ReuseAddr(m_socket);
      if ( nonblock )
        NonBlock(m_socket);

      if ( ::connect(m_socket, p->ai_addr, p->ai_addrlen) == -1 ) 
      {
          /* If the socket is non-blocking, it is ok for connect() to
            * return an EINPROGRESS error here. */
          if ( errno == EINPROGRESS && nonblock )
          {
            closeSocketIfError.disable();
            return;
          }

          close(m_socket);
          m_socket = SOCKET_NOT_CONNECTED;
          continue;
      }

      closeSocketIfError.disable();
      return;
  }
  if ( p == NULL )
    throw std::runtime_error("unable to create socket");
}

SocketType Socket::getSocket() const
{
  return m_socket;
}

Socket::~Socket()
{
  if ( m_socket != SOCKET_NOT_CONNECTED )
    close(m_socket);
}

ServerSocket::ServerSocket()
  : m_serverSocket(SOCKET_NOT_CONNECTED),
  m_clientSocket(SOCKET_NOT_CONNECTED)
{
}

void ServerSocket::listen(const char* bindingAddr, int port, int backlog)
{
  char portAsStr[100];
  sprintf(portAsStr, "%d", port);
  addrinfo hints;
  memset(&hints,0,sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  addrinfo* servinfo = NULL;
  FreeAddrInfo freeAddrInfo(&servinfo);
  if ( ::getaddrinfo(bindingAddr, portAsStr, &hints, &servinfo) != 0 )
    throw std::runtime_error("getaddrinfo");

  addrinfo* p = NULL;
  for ( p = servinfo; p; p = p->ai_next ) 
  {
    if ( ( m_serverSocket = ::socket(p->ai_family,p->ai_socktype,p->ai_protocol)) == -1 )
      continue;

    ReuseAddr(m_serverSocket);
    Listen(m_serverSocket, p->ai_addr, p->ai_addrlen, backlog);
  }
}

void ServerSocket::accept(std::string* ip, int* port) 
{
  struct sockaddr_storage sa;
  socklen_t salen = sizeof(sa);
  while ( 1 ) 
  {
    m_clientSocket = ::accept(m_serverSocket, (struct sockaddr*)&sa, &salen);
    if ( m_clientSocket == -1 )
    {
      if ( errno == EINTR )
          continue;
      else 
      {
        throw std::runtime_error("accept error");
      }
    }
    break;
  }

  if ( sa.ss_family == AF_INET ) 
  {
    sockaddr_in* s = (sockaddr_in *)&sa;
    if ( ip ) 
    {
      char ipStr[INET_ADDRSTRLEN + 1] = { 0 };
      ::inet_ntop(AF_INET, (void*)&(s->sin_addr), ipStr, INET_ADDRSTRLEN);
      *ip = ipStr;
    }

    if ( port ) 
      *port = ::ntohs(s->sin_port);
  } 
  else 
  {
    sockaddr_in6* s = (sockaddr_in6 *)&sa;
    if ( ip ) 
    {
      char ipStr[INET6_ADDRSTRLEN + 1] = { 0 };
      ::inet_ntop(AF_INET6, (void*)&(s->sin6_addr), ipStr, INET6_ADDRSTRLEN);
      *ip = ipStr;
    }
    if ( port ) 
      *port = ::ntohs(s->sin6_port);
  }
}

SocketType ServerSocket::getClientSocket() const
{
  return m_clientSocket;
}

ServerSocket::~ServerSocket()
{
  if ( m_clientSocket != SOCKET_NOT_CONNECTED )
    close(m_clientSocket);

  if ( m_serverSocket != SOCKET_NOT_CONNECTED )
    close(m_serverSocket);
}

