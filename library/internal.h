#ifndef INTERNAL_H
#define INTERNAL_H
#ifdef __cplusplus
extern "C"
{
#endif
#include "nex.h"
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
    struct sockaddr_in addr;
    int id;
    bool isClosed;
    void *context;
    void (*contextDeleter)(void *);
  } ConnectedPeer;

  typedef struct
  {
    Socket socket;
    SocketType socketType;
    ConnectionType connectionType;

    union
    {
      void (*onClientData)(Data, socket_t);
      void (*onServerData)(Data);
      void (*onPeerData)(Data, int);
    } callback;

    pthread_mutex_t lock;
    bool initialized;
    char lastError[256];

    // tcp specific fields
    //  server specific fields
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

    // udp specific fields
    struct
    {
      pthread_t *peerThreads;
      ConnectedPeer *peers;
      int maxPeers;
      int numPeers;
      bool listening;
      int idIncrementer;
    } peer;

  } NetworkContext;

  extern NetworkContext networkContext;

  void removeClient(int i);
  void removeAllClients();
  void removePeer(int i);
  void removeAllPeers();

#ifdef __cplusplus
}
#endif

#endif