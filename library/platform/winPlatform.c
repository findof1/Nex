#include "platform.h"

#ifdef PLATFORM_WINDOWS

struct sockaddr_in createSockaddrIn(int port, const char *ipAddress)
{
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;

  if (inet_pton(AF_INET, ipAddress, &addr.sin_addr) <= 0)
  {
    printf("Invalid IP address: %s\n", ipAddress);
    addr.sin_addr.s_addr = INADDR_ANY;
  }

  addr.sin_port = htons(port);
  return addr;
}

int platformInit()
{
  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
  {
    printf("WSAStartup failed with error: %d\n", WSAGetLastError());
    return PLATFORM_FAILURE;
  }
  return PLATFORM_SUCCESS;
}

void platformCleanup()
{
  WSACleanup();
}

socket_t createSocket(int type, int protocol)
{
  socket_t sock_fd = socket(AF_INET, type, protocol);
  if (sock_fd == INVALID_SOCKET)
  {
    WSACleanup();
    return PLATFORM_FAILURE;
  }
  return sock_fd;
}

int bindSocket(socket_t socket, const struct sockaddr *addr, socklen_t addrlen)
{
  if (bind(socket, addr, addrlen) == SOCKET_ERROR)
  {
    closesocket(socket);
    WSACleanup();
    return PLATFORM_FAILURE;
  }
  return PLATFORM_SUCCESS;
}

int listenSocket(socket_t socket, int maxClients)
{
  if (listen(socket, maxClients) == SOCKET_ERROR)
  {
    closesocket(socket);
    WSACleanup();
    return PLATFORM_FAILURE;
  }
  return PLATFORM_SUCCESS;
}

socket_t acceptSocket(socket_t socket, struct sockaddr *addr, socklen_t *addrlen)
{
  socket_t clientSocket = accept(socket, addr, addrlen);
  if (clientSocket == INVALID_SOCKET)
  {
    printf("Accept failed\n");
    return PLATFORM_FAILURE;
  }
  return clientSocket;
}

int connectSocket(socket_t socket, const struct sockaddr *addr, socklen_t addrlen)
{
  if (connect(socket, addr, addrlen) == SOCKET_ERROR)
  {
    printf("Connect failed. Error code: %d\n", WSAGetLastError());
    return PLATFORM_FAILURE;
  }
  return PLATFORM_SUCCESS;
}

int sendData(socket_t socket, const void *buf, size_t len, int flags)
{
  if (socket == INVALID_SOCKET)
  {
    return PLATFORM_FAILURE;
  }

  if (send(socket, buf, len, flags) == SOCKET_ERROR)
  {
    printf("Send failed to client socket %d. Error: %d\n", socket, WSAGetLastError());
    return PLATFORM_FAILURE;
  }
  return PLATFORM_SUCCESS;
}

int recvData(socket_t socket, void *buf, size_t len, int flags)
{
  if (socket == INVALID_SOCKET)
  {
    return PLATFORM_FAILURE;
  }

  int bytesReceived = recv(socket, buf, len, flags);
  if (bytesReceived == SOCKET_ERROR)
  {
    int err = WSAGetLastError();

    if (err == WSAECONNRESET)
    {
      return PLATFORM_CONNECTION_CLOSED;
    }

    printf("Receive failed from socket %d. Error: %d\n", socket, WSAGetLastError());
    return PLATFORM_FAILURE;
  }

  if (bytesReceived == 0)
  {
    printf("Connection closed by client on socket %d\n", socket);
    return PLATFORM_CONNECTION_CLOSED;
  }

  return bytesReceived;
}

int recvAll(socket_t socket, void *buf, size_t len, int flags)
{
  size_t totalReceived = 0;
  char *buffer = (char *)buf;

  while (totalReceived < len)
  {
    int bytesReceived = recvData(socket, buffer + totalReceived, len - totalReceived, flags);
    if (bytesReceived == PLATFORM_FAILURE)
    {
      printf("recvAll: Failed while receiving data\n");
      return PLATFORM_FAILURE;
    }
    if (bytesReceived == PLATFORM_CONNECTION_CLOSED)
    {
      printf("recvAll: Connection closed unexpectedly\n");
      return PLATFORM_FAILURE;
    }

    totalReceived += bytesReceived;
  }

  return totalReceived;
}

int platformGetLastError()
{
  return WSAGetLastError();
}

void shutdownRead(socket_t socket)
{
  shutdown(socket, SD_RECEIVE);
}

void shutdownWrite(socket_t socket)
{
  shutdown(socket, SD_SEND);
}

void shutdownBoth(socket_t socket)
{
  shutdown(socket, SD_BOTH);
}

int closeSocket(socket_t socket)
{
  if (closesocket(socket) == SOCKET_ERROR)
  {
    printf("Socket closure failed. Error: %d\n", WSAGetLastError());
    return PLATFORM_FAILURE;
  }
  return PLATFORM_SUCCESS;
}

#endif