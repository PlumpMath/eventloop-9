#include <unistd.h>
#include <stdexcept>
#include <stdio.h>
#include "Socket.h"
#include "RunLoop.h"
#include "string.h"

void readFromStdin(RunLoop* runloop, int fd);

class StdWriteRead : public ReadReady, WriteReady
{
public:
  StdWriteRead(SocketType socket)
    : m_socket(socket)
  {
  }
  virtual void doWrite(RunLoop* runloop, int fd)
  {
    write(fd, buf, strlen(buf));
    runloop->registerReadIntent(m_socket, this);
  }

  virtual void doRead(RunLoop* runloop, int fd)
  {
    read(fd, buf, 1000);
    runloop->registerWriteIntent(m_socket, this);
  }
private:
  SocketType m_socket;
  char buf[1000];
};

int main()
{
  /*
  StdWriteRead stdWriteRead;
  runloop.registerReadIntent(fileno(stdin), &stdWriteRead);
  runloop.run();
  */
  /*
  try
  {
    RunLoop runloop;
    Socket socket;
    socket.connect("localhost", 22, false);
    int sock = socket.getSocket();
    StdWriteRead stdWriteRead(sock);
    runloop.registerReadIntent(sock, &stdWriteRead);
    runloop.run();
  } 
  catch ( std::runtime_error r )
  {
    fprintf(stderr, r.what());
  }
  */
  ServerSocket serverSocket;
  serverSocket.listen("0.0.0.0", 3000, 10);
  std::string ip;
  serverSocket.accept(&ip, NULL);
  printf("socket: %s\n", ip.c_str());
}

