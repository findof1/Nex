#include "internal.h"
NetworkContext networkContext;

void removeClient(int i)
{
  pthread_mutex_lock(&networkContext.lock);

  if (i < 0 || i >= networkContext.server.numClients)
  {
    snprintf(networkContext.lastError, sizeof(networkContext.lastError), "Invalid client index");
    pthread_mutex_unlock(&networkContext.lock);
    return;
  }

  ServerClient *client = &networkContext.server.clients[i];

  if (client->context && client->contextDeleter)
  {
    client->contextDeleter(client->context);
    client->context = NULL;
    client->contextDeleter = NULL;
  }

  if (!client->isClosed)
  {
    closeSocket(client->socket.socket);
    client->isClosed = true;
  }

  pthread_cancel(networkContext.server.clientThreads[i]);
  pthread_join(networkContext.server.clientThreads[i], NULL);

  for (int j = i; j < networkContext.server.numClients - 1; j++)
  {
    networkContext.server.clients[j] = networkContext.server.clients[j + 1];
    networkContext.server.clientThreads[j] = networkContext.server.clientThreads[j + 1];
  }

  int last = networkContext.server.numClients - 1;
  memset(&networkContext.server.clients[last], 0, sizeof(ServerClient));
  networkContext.server.clientThreads[last] = 0;

  networkContext.server.numClients--;

  pthread_mutex_unlock(&networkContext.lock);
}

void removeAllClients()
{
  pthread_mutex_lock(&networkContext.lock);

  int numClients = networkContext.server.numClients;

  for (int i = 0; i < numClients; i++)
  {
    ServerClient *client = &networkContext.server.clients[i];

    if (client->context && client->contextDeleter)
    {
      client->contextDeleter(client->context);
      client->context = NULL;
      client->contextDeleter = NULL;
    }

    if (!client->isClosed)
    {
      closeSocket(client->socket.socket);
      client->isClosed = true;
    }

    pthread_cancel(networkContext.server.clientThreads[i]);
    pthread_join(networkContext.server.clientThreads[i], NULL);
  }

  memset(networkContext.server.clients, 0, sizeof(networkContext.server.clients));
  memset(networkContext.server.clientThreads, 0, sizeof(networkContext.server.clientThreads));

  networkContext.server.numClients = 0;

  pthread_mutex_unlock(&networkContext.lock);
}

void removePeer(int i)
{
  pthread_mutex_lock(&networkContext.lock);

  if (i < 0 || i >= networkContext.peer.numPeers)
  {
    snprintf(networkContext.lastError, sizeof(networkContext.lastError), "Invalid client index");
    pthread_mutex_unlock(&networkContext.lock);
    return;
  }

  ConnectedPeer *peer = &networkContext.peer.peers[i];

  if (peer->context && peer->contextDeleter)
  {
    peer->contextDeleter(peer->context);
    peer->context = NULL;
    peer->contextDeleter = NULL;
  }

  if (!peer->isClosed)
  {
    peer->isClosed = true;
  }
  peer->id = -1;

  pthread_cancel(networkContext.peer.peerThreads[i]);
  pthread_join(networkContext.peer.peerThreads[i], NULL);

  for (int j = i; j < networkContext.peer.numPeers - 1; j++)
  {
    networkContext.peer.peers[j] = networkContext.peer.peers[j + 1];
    networkContext.peer.peerThreads[j] = networkContext.peer.peerThreads[j + 1];
  }

  int last = networkContext.peer.numPeers - 1;
  memset(&networkContext.peer.peers[last], 0, sizeof(ConnectedPeer));
  networkContext.peer.peerThreads[last] = 0;

  networkContext.peer.numPeers--;

  pthread_mutex_unlock(&networkContext.lock);
}

void removeAllPeers()
{
  pthread_mutex_lock(&networkContext.lock);

  int numClients = networkContext.peer.numPeers;

  for (int i = 0; i < numClients; i++)
  {
    ConnectedPeer *peer = &networkContext.peer.peers[i];

    if (peer->context && peer->contextDeleter)
    {
      peer->contextDeleter(peer->context);
      peer->context = NULL;
      peer->contextDeleter = NULL;
    }

    if (!peer->isClosed)
    {
      peer->isClosed = true;
    }
    peer->id = -1;

    pthread_cancel(networkContext.peer.peerThreads[i]);
    pthread_join(networkContext.peer.peerThreads[i], NULL);
  }

  memset(networkContext.peer.peers, 0, sizeof(networkContext.peer.peers));
  memset(networkContext.peer.peerThreads, 0, sizeof(networkContext.peer.peerThreads));

  networkContext.peer.numPeers = 0;

  pthread_mutex_unlock(&networkContext.lock);
}