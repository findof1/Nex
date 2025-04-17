#ifndef INTERNAL_H
#define INTERNAL_H

#include "core.h"
#include <pthread.h>
typedef struct
{
  Socket socket;
  bool active;
  int id;
} ServerClient;

typedef struct
{
  Socket socket;
  SocketType socketType;
  ConnectionType connectionType;

  union
  {
    void (*onClientData)(Socket *, const char *, size_t);
    void (*onServerData)(const char *, size_t);
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
    Socket serverSocket;
    pthread_t serverThread;
  } client;
} NetworkContext;

extern NetworkContext networkContext;

void removeClient(int i); // WARNING: DO NOT CALL ON CLIENT THREADS, trust me bro

#endif