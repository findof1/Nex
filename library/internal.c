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

  closeSocket(client->socket.socket);

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