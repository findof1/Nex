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

int listenSocket(socket_t sockfd, int maxClients)
{
  if (listen(sockfd, maxClients) == SOCKET_ERROR)
  {
    printf("Listen failed\n");
    closesocket(sockfd);
    WSACleanup();
    return PLATFORM_FAILURE;
  }
  return PLATFORM_SUCCESS;
}

socket_t acceptSocket(socket_t sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
  socket_t clientSocket = accept(sockfd, addr, addrlen);
  if (clientSocket == INVALID_SOCKET)
  {
    printf("Accept failed\n");
    return PLATFORM_FAILURE;
  }
  return clientSocket;
}

int connectSocket(socket_t sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
  if (connect(sockfd, addr, addrlen) == SOCKET_ERROR)
  {
    printf("Connect failed. Error code: %d\n", WSAGetLastError());
    return PLATFORM_FAILURE;
  }
  return PLATFORM_SUCCESS;
}

int sendData(socket_t sockfd, const void *buf, size_t len, int flags)
{
  if (sockfd == INVALID_SOCKET)
  {
    return PLATFORM_FAILURE;
  }

  if (send(sockfd, buf, len, flags) == SOCKET_ERROR)
  {
    printf("Send failed to client socket %d. Error: %d\n", sockfd, WSAGetLastError());
    return PLATFORM_FAILURE;
  }
  return PLATFORM_SUCCESS;
}

int receiveData(socket_t sockfd, void *buf, size_t len, int flags)
{
  if (sockfd == INVALID_SOCKET)
  {
    return PLATFORM_FAILURE;
  }

  int bytesReceived = recv(sockfd, buf, len, flags);
  if (bytesReceived == SOCKET_ERROR)
  {
    printf("Receive failed from socket %d. Error: %d\n", sockfd, WSAGetLastError());
    return PLATFORM_FAILURE;
  }

  if (bytesReceived == 0)
  {
    printf("Connection closed by client on socket %d\n", sockfd);
    return PLATFORM_CONNECTION_CLOSED;
  }

  return bytesReceived;
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

int closeSocket(socket_t sockfd)
{
  if (closesocket(sockfd) == SOCKET_ERROR)
  {
    printf("Socket closure failed. Error: %d\n", WSAGetLastError());
    return PLATFORM_FAILURE;
  }
  return PLATFORM_SUCCESS;
}

#endif