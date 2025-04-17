#include "core.h"
#include "internal.h"

int init(ConnectionType connectionType, SocketType socketType)
{
  memset(&networkContext, 0, sizeof(NetworkContext));

  if (pthread_mutex_init(&networkContext.lock, NULL) != 0)
  {
    strncpy(networkContext.lastError, "Thread Mutex Failed to Initialize", sizeof(networkContext.lastError) - 1);
    networkContext.lastError[sizeof(networkContext.lastError) - 1] = '\0';
    return NETWORK_ERR_INTIALIZATION;
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
    networkContext.server.listening = false;
  }
  else if (socketType == Client)
  {
    networkContext.client.serverSocket.socket = -1;
    networkContext.client.serverThread = 0;
  }
  else
  {
    strncpy(networkContext.lastError, "Invalid Socket Type Passed Into: int init(ConnectionType connectionType, SocketType socketType)", sizeof(networkContext.lastError) - 1);
    networkContext.lastError[sizeof(networkContext.lastError) - 1] = '\0';
    return NETWORK_ERR_INVALID;
  }

  if (platformInit() != PLATFORM_SUCCESS)
  {
    strncpy(networkContext.lastError, "Platform initialization failed", sizeof(networkContext.lastError) - 1);
    networkContext.lastError[sizeof(networkContext.lastError) - 1] = '\0';
    return NETWORK_ERR_INTIALIZATION;
  }

  return 0;
}

int startServer(int port, int maxClients, void (*onClientData)(Socket *, const char *, size_t))
{
  if (maxClients <= 0 || onClientData == NULL)
  {
    strncpy(networkContext.lastError, "Invalid max clients or client data passed into: int startServer(int port, int maxClients, void (*onClientData)(Socket *, const char *, size_t))", sizeof(networkContext.lastError) - 1);
    networkContext.lastError[sizeof(networkContext.lastError) - 1] = '\0';
    return NETWORK_ERR_INVALID;
  }

  if (!networkContext.initialized)
  {
    strncpy(networkContext.lastError, "Platform Is Not Initialized!", sizeof(networkContext.lastError) - 1);
    networkContext.lastError[sizeof(networkContext.lastError) - 1] = '\0';
    return NETWORK_ERR_INVALID;
  }

  networkContext.server.maxClients = maxClients;
  networkContext.callback.onClientData = onClientData;

  networkContext.server.clients = calloc(maxClients, sizeof(ServerClient));
  networkContext.server.clientThreads = calloc(maxClients, sizeof(pthread_t));
  if (!networkContext.server.clients || !networkContext.server.clientThreads)
  {
    strncpy(networkContext.lastError, "Out of memory allocating client arrays", sizeof(networkContext.lastError) - 1);
    networkContext.lastError[sizeof(networkContext.lastError) - 1] = '\0';
    return NETWORK_ERR_UNKNOWN;
  }

  networkContext.socket.socket = createSocket(SOCK_STREAM, IPPROTO_TCP);
  if (networkContext.socket.socket == PLATFORM_FAILURE)
  {
    strncpy(networkContext.lastError, "Socket creation failed\n", sizeof(networkContext.lastError) - 1);
    networkContext.lastError[sizeof(networkContext.lastError) - 1] = '\0';
    return NETWORK_ERR_SOCKET;
  }

  networkContext.socket.serverAddr = createSockaddrIn(8080, "0.0.0.0");
  if (bindSocket(networkContext.socket.socket, (struct sockaddr *)&networkContext.socket.serverAddr, sizeof(networkContext.socket.serverAddr)) == PLATFORM_FAILURE)
  {
    strncpy(networkContext.lastError, "Socket bind failed\n", sizeof(networkContext.lastError) - 1);
    networkContext.lastError[sizeof(networkContext.lastError) - 1] = '\0';
    return NETWORK_ERR_BIND;
  }

  if (listenSocket(networkContext.socket.socket, 1) == PLATFORM_FAILURE)
  {
    strncpy(networkContext.lastError, "Listen failed\n", sizeof(networkContext.lastError) - 1);
    networkContext.lastError[sizeof(networkContext.lastError) - 1] = '\0';
    return NETWORK_ERR_LISTEN;
  }

  if (pthread_create(&networkContext.server.acceptThread, NULL, serverAcceptLoop, NULL) != 0)
  {
    strncpy(networkContext.lastError, "pthread_create acceptLoop failed", sizeof(networkContext.lastError) - 1);
    networkContext.lastError[sizeof(networkContext.lastError) - 1] = '\0';
    return NETWORK_ERR_THREAD;
  }

  return NETWORK_OK;
}

static int serverAcceptLoop()
{
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