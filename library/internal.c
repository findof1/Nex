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
    networkContext.server.clients[j].id = j;
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