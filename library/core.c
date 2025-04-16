#include "core.h"
#include "internal.h"

int init(ConnectionType connectionType, SocketType socketType)
{
  memset(&networkContext, 0, sizeof(NetworkContext));

  if (pthread_mutex_init(&networkContext.lock, NULL) != 0)
  {
    strncpy(networkContext.lastError, "Thread Mutex Failed to Initialize", sizeof(networkContext.lastError) - 1);
    networkContext.lastError[sizeof(networkContext.lastError) - 1] = '\0';
    return NETWORK_ERR_UNKNOWN;
  }

  networkContext.connectionType = connectionType;
  networkContext.socketType = socketType;
  networkContext.initialized = true;

  if (socketType == Server)
  {
    networkContext.server.clientThreads = NULL;
    networkContext.server.clients = NULL;
    networkContext.server.maxClients = 0;
    networkContext.server.numClients = 0;
  }
  else if (socketType == Client)
  {
    networkContext.client.serverSocket.socket = -1;
    networkContext.client.serverThread = NULL;
  }
  else
  {
    strncpy(networkContext.lastError, "Invalid Socket Type Passed Into: int init(ConnectionType connectionType, SocketType socketType)", sizeof(networkContext.lastError) - 1);
    networkContext.lastError[sizeof(networkContext.lastError) - 1] = '\0';
    return NETWORK_ERR_INVALID;
  }

  return 0;
}

void printLastError()
{
  if (strlen(networkContext.lastError) > 0)
  {
    printf("Error: %s\n", networkContext.lastError);
  }
  else
  {
    printf("No error occurred.\n");
  }
}