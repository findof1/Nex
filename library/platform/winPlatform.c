#include "platform.h"

#ifdef PLATFORM_WINDOWS

socket_t createSocket(int domain, int type, int protocol)
{
  socket_t sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock_fd == INVALID_SOCKET)
  {
    printf("Socket creation failed\n");
    WSACleanup();
    return PLATFORM_FAILURE;
  }
  return sock_fd;
}

int bindSocket(socket_t sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
  if (bind(sockfd, addr, addrlen) == SOCKET_ERROR)
  {
    printf("Socket bind failed\n");
    closesocket(sockfd);
    WSACleanup();
    return PLATFORM_FAILURE;
  }
  return PLATFORM_SUCCESS;
}

int listenSocket(socket_t sockfd, int backlog)
{
}

socket_t acceptSocket(socket_t sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
}

int connectSocket(socket_t sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
}

int sendData(socket_t sockfd, const void *buf, size_t len, int flags)
{
}

int receiveData(socket_t sockfd, void *buf, size_t len, int flags)
{
}

int platformGetLastError()
{
  return WSAGetLastError();
}

void shutdownRead(socket_t sockfd)
{
  shutdown(sockfd, SD_RECEIVE);
}

void shutdownWrite(socket_t sockfd)
{
  shutdown(sockfd, SD_SEND);
}

void shutdownBoth(socket_t sockfd)
{
  shutdown(sockfd, SD_BOTH);
}

#endif