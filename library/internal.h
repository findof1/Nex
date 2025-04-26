#ifndef INTERNAL_H
#define INTERNAL_H

#include "core.h"
#include <pthread.h>
typedef struct
{
  Socket socket;
  bool isClosed;
  void *context;
  void (*contextDeleter)(void *);
} ServerClient;

typedef struct
{
  Socket socket;
  SocketType socketType;
  ConnectionType connectionType;

  union
  {
    void (*onClientData)(Data, socket_t);
    void (*onServerData)(Data);
  } callback;

  pthread_mutex_t lock;
  bool initialized;
  char lastError[256];

  // server specific fields
  struct
  {
    pthread_t *clientThreads;
    ServerClient *clients;
    pthread_t acceptThread;
    int maxClients;
    int numClients;
    bool listening;
  } server;

  // client specific fields
  struct
  {
    pthread_t serverThread;
    bool running;
  } client;
} NetworkContext;

extern NetworkContext networkContext;

void removeClient(int i);
void removeAllClients();

#endif