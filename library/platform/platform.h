#ifndef PLATFORM_H
#define PLATFORM_H
#include <stdio.h>

#define PLATFORM_FAILURE -1
#define PLATFORM_SUCCESS 1
#define PLATFORM_CONNECTION_CLOSED 0

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
#elif PLATFORM_LINUX
typedef int socket_t;
#else
#error "Unsupported platform"
#endif

struct sockaddr_in createSockaddrIn(int port, const char *ipAddress);
int platformInit();
void platformCleanup();
socket_t createSocket(int type, int protocol);
int bindSocket(socket_t socket, const struct sockaddr *addr, socklen_t addrlen);
int listenSocket(socket_t socket, int maxClients);
socket_t acceptSocket(socket_t socket, struct sockaddr *addr, socklen_t *addrlen);
int connectSocket(socket_t socket, const struct sockaddr *addr, socklen_t addrlen);
int sendData(socket_t socket, const void *buf, size_t len, int flags);
int recvData(socket_t socket, void *buf, size_t len, int flags);

int platformGetLastError();

void shutdownRead(socket_t socket);
void shutdownWrite(socket_t socket);
void shutdownBoth(socket_t socket);

int closeSocket(socket_t socket);

#endif