#include "platform.h"

#ifdef PLATFORM_LINUX

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

struct sockaddr_in createSockaddrIn(int port, const char *ipAddress)
{
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;

  if (inet_pton(AF_INET, ipAddress, &addr.sin_addr) <= 0)
  {
    fprintf(stderr, "Invalid IP address: %s\n", ipAddress);
    addr.sin_addr.s_addr = INADDR_ANY;
  }

  addr.sin_port = htons(port);
  return addr;
}

int platformInit()
{
  return PLATFORM_SUCCESS;
}

void platformCleanup()
{
}

socket_t createSocket(int type, int protocol)
{
  socket_t sock_fd = socket(AF_INET, type, protocol);
  if (sock_fd < 0)
  {
    perror("socket");
    return PLATFORM_FAILURE;
  }
  return sock_fd;
}

int bindSocket(socket_t sock, const struct sockaddr *addr, socklen_t addrlen)
{
  if (bind(sock, addr, addrlen) < 0)
  {
    perror("bind");
    close(sock);
    return PLATFORM_FAILURE;
  }
  return PLATFORM_SUCCESS;
}

int listenSocket(socket_t sock, int maxClients)
{
  if (listen(sock, maxClients) < 0)
  {
    perror("listen");
    close(sock);
    return PLATFORM_FAILURE;
  }
  return PLATFORM_SUCCESS;
}

socket_t acceptSocket(socket_t sock, struct sockaddr *addr, socklen_t *addrlen)
{
  socket_t clientSock = accept(sock, addr, addrlen);
  if (clientSock < 0)
  {
    perror("accept");
    return PLATFORM_FAILURE;
  }
  return clientSock;
}

int connectSocket(socket_t sock, const struct sockaddr *addr, socklen_t addrlen)
{
  if (connect(sock, addr, addrlen) < 0)
  {
    perror("connect");
    return PLATFORM_FAILURE;
  }
  return PLATFORM_SUCCESS;
}

int sendData(socket_t sock, const void *buf, size_t len, int flags)
{
  if (sock < 0)
    return PLATFORM_FAILURE;

  ssize_t sent = send(sock, buf, len, flags);
  if (sent < 0)
  {
    perror("send");
    return PLATFORM_FAILURE;
  }
  return PLATFORM_SUCCESS;
}

int recvData(socket_t sock, void *buf, size_t len, int flags)
{
  if (sock < 0)
    return PLATFORM_FAILURE;

  ssize_t recvd = recv(sock, buf, len, flags);
  if (recvd < 0)
  {
    if (errno == ECONNRESET || errno == ENOTCONN)
      return PLATFORM_CONNECTION_CLOSED;
    perror("recv");
    return PLATFORM_FAILURE;
  }
  if (recvd == 0)
  {
    return PLATFORM_CONNECTION_CLOSED;
  }
  return (int)recvd;
}

int recvAll(socket_t sock, void *buf, size_t len, int flags)
{
  size_t total = 0;
  char *p = buf;

  while (total < len)
  {
    int ret = recvData(sock, p + total, len - total, flags);
    if (ret <= 0)
      return PLATFORM_FAILURE;
    total += ret;
  }
  return (int)total;
}

int sendDataTo(socket_t sock, const void *buf, size_t len, int flags, const struct sockaddr_in *destAddr)
{
  ssize_t sent = sendto(sock,
                        buf,
                        len,
                        flags,
                        (const struct sockaddr *)destAddr,
                        sizeof(*destAddr));
  if (sent < 0)
  {
    perror("sendto");
    return PLATFORM_FAILURE;
  }
  return (int)sent;
}

int recvDataFrom(socket_t sock, void *buf, size_t len, int flags, struct sockaddr_in *srcAddr)
{
  socklen_t addrLen = sizeof(*srcAddr);
  ssize_t recvd = recvfrom(sock,
                           buf,
                           len,
                           flags,
                           (struct sockaddr *)srcAddr,
                           &addrLen);
  if (recvd < 0)
  {
    perror("recvfrom");
    return PLATFORM_FAILURE;
  }
  return (int)recvd;
}

int platformGetLastError()
{
  return errno;
}

void shutdownRead(socket_t sock)
{
  shutdown(sock, SHUT_RD);
}
void shutdownWrite(socket_t sock)
{
  shutdown(sock, SHUT_WR);
}
void shutdownBoth(socket_t sock)
{
  shutdown(sock, SHUT_RDWR);
}

int closeSocket(socket_t sock)
{
  if (sock < 0)
    return PLATFORM_SUCCESS;

  if (close(sock) < 0)
  {
    perror("close");
    return PLATFORM_FAILURE;
  }
  return PLATFORM_SUCCESS;
}

#endif