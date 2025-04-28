#ifndef CORE_H
#define CORE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "platform.h"
#include "stdbool.h"
#include "serialization.h"

  typedef enum
  {
    CONNECTION_TCP,
    CONNECTION_UDP
  } ConnectionType;

  typedef enum
  {
    NETWORK_OK = 0,
    NETWORK_ERR_SOCKET,
    NETWORK_ERR_BIND,
    NETWORK_ERR_LISTEN,
    NETWORK_ERR_ACCEPT,
    NETWORK_ERR_CONNECT,
    NETWORK_ERR_SEND,
    NETWORK_ERR_RECV,
    NETWORK_ERR_TIMEOUT,
    NETWORK_ERR_THREAD,
    NETWORK_ERR_INTIALIZATION,
    NETWORK_ERR_INVALID,
    NETWORK_ERR_UNKNOWN
  } NetworkError;

  typedef enum
  {
    Client,
    Server
  } SocketType;

  typedef struct
  {
    socket_t socket;
    struct sockaddr_in addr;
  } Socket;

  int init(ConnectionType connectionType, SocketType socketType);
  void printLastError();
  const char *getLastError();
  int startServer(int port, int maxClients, void (*onClientData)(Data, socket_t));
  int connectToServer(const char *ip, int port, void (*onServerData)(Data));
  int sendToAllClients(Data data);
  int broadcastToClients(Data data, socket_t sender);
  int sendToClient(Data data, socket_t client);
  int setClientContext(void *context, socket_t client, void (*deleter)(void *));
  void *getClientContext(socket_t client);
  int sendToServer(Data data);
  int shutdownNetwork();

#ifdef __cplusplus
}
#endif

#endif