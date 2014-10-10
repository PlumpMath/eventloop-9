#ifndef MRPC_NET_H
#define MRPC_NET_H

#define MRPC_OK 0
#define MRPC_ERROR -1

#ifndef WIN32
typedef int SocketType;
#else
#include <windows.h>
typedef SOCKET SocketType;
#endif

#endif

