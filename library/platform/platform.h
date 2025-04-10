#ifndef PLATFORM_H
#define PLATFORM_H
#include <stdio.h>

#define PLATFORM_FAILURE -1
#define PLATFORM_SUCCESS 1

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#define PLATFORM_WINDOWS 1
#define PLATFORM_LINUX 0
#elif __linux__
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#define PLATFORM_WINDOWS 0
#define PLATFORM_LINUX 1
#else
#error "Unsupported platform"
#endif

#ifdef PLATFORM_WINDOWS
typedef SOCKET socket_t;
const socket_t INVALID_SOCKET_VALUE = INVALID_SOCKET;
const int SOCKET_ERROR_VALUE = SOCKET_ERROR;
const int ERROR_NONE = 0;
#elif PLATFORM_LINUX
typedef int socket_t;
const socket_t INVALID_SOCKET_VALUE = -1;
const int SOCKET_ERROR_VALUE = -1;
const int ERROR_NONE = 0;
#else
#error "Unsupported platform"
#endif

#ifdef PLATFORM_WINDOWS
bool platformInit()
{
  WSADATA wsaData;
  return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
}

void platformCleanup()
{
  WSACleanup();
}
#elif PLATFORM_LINUX
inline bool platformInit() { return true; }
inline void platformCleanup() {}
#endif

struct sockaddr_in createSockaddrIn(int port, const char *ipAddress = "0.0.0.0")
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

socket_t createSocket(int domain, int type, int protocol);
int bindSocket(socket_t sockfd, const struct sockaddr *addr, socklen_t addrlen);
int listenSocket(socket_t sockfd, int backlog);
socket_t acceptSocket(socket_t sockfd, struct sockaddr *addr, socklen_t *addrlen);
int connectSocket(socket_t sockfd, const struct sockaddr *addr, socklen_t addrlen);
int sendData(socket_t sockfd, const void *buf, size_t len, int flags);
int receiveData(socket_t sockfd, void *buf, size_t len, int flags);

int platformGetLastError();

void shutdownRead(socket_t sockfd);
void shutdownWrite(socket_t sockfd);
void shutdownBoth(socket_t sockfd);

#endif