#include "core.h"
#include "internal.h"

static void *serverAcceptLoop(void *arg);
static void *clientDataLoop(void *arg);

int init(ConnectionType connectionType, SocketType socketType)
{
  if (platformInit() != PLATFORM_SUCCESS)
  {
    strncpy(networkContext.lastError, "Platform initialization failed", sizeof(networkContext.lastError) - 1);
    networkContext.lastError[sizeof(networkContext.lastError) - 1] = '\0';
    return NETWORK_ERR_INTIALIZATION;
  }

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

  return NETWORK_OK;
}

int startServer(int port, int maxClients, void (*onClientData)(Data))
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

  networkContext.socket.addr = createSockaddrIn(8080, "0.0.0.0");
  if (bindSocket(networkContext.socket.socket, (struct sockaddr *)&networkContext.socket.addr, sizeof(networkContext.socket.addr)) == PLATFORM_FAILURE)
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

typedef struct
{
  ServerClient *client;
  int clientIndex;
} ClientThreadArgs;

static void *serverAcceptLoop(void *arg)
{
  while (true)
  {
    pthread_mutex_lock(&networkContext.lock);

    if (networkContext.server.numClients >= networkContext.server.maxClients)
    {
      strncpy(networkContext.lastError, "Max clients reached\n", sizeof(networkContext.lastError) - 1);
      networkContext.lastError[sizeof(networkContext.lastError) - 1] = '\0';
      pthread_mutex_unlock(&networkContext.lock);
      continue;
    }

    int clientIndex = networkContext.server.numClients;

    pthread_mutex_unlock(&networkContext.lock);

    struct sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    Socket clientSocket;
    clientSocket.socket = acceptSocket(networkContext.socket.socket, (struct sockaddr *)&clientAddr, &addrLen);
    if (clientSocket.socket < 0)
    {
      strncpy(networkContext.lastError, "Client accept failed\n", sizeof(networkContext.lastError) - 1);
      networkContext.lastError[sizeof(networkContext.lastError) - 1] = '\0';
      continue;
    }

    pthread_mutex_lock(&networkContext.lock);

    ServerClient *client = &networkContext.server.clients[clientIndex];
    client->socket = clientSocket;
    client->active = true;
    client->id = clientIndex;

    pthread_t thread;
    ClientThreadArgs *args = malloc(sizeof(ClientThreadArgs));
    args->client = client;
    args->clientIndex = clientIndex;
    int result = pthread_create(&thread, NULL, clientDataLoop, args);
    networkContext.server.clientThreads[clientIndex] = thread;

    if (result != 0)
    {
      strncpy(networkContext.lastError, "Failed to create thread\n", sizeof(networkContext.lastError) - 1);
      networkContext.lastError[sizeof(networkContext.lastError) - 1] = '\0';
      closeSocket(client->socket.socket);
      client->active = false;
      removeClient(clientIndex);
    }
    else
    {
      pthread_detach(thread);
      networkContext.server.numClients++;
    }
    pthread_mutex_unlock(&networkContext.lock);
  }
}

static void *clientDataLoop(void *arg)
{
  ClientThreadArgs *args = (ClientThreadArgs *)arg;
  ServerClient *client = args->client;
  int clientIndex = args->clientIndex;

  while (1)
  {
    pthread_mutex_lock(&networkContext.lock);
    char buffer[1000];
    int bytesReceived = recvData(client->socket.socket, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived > 0)
    {
      buffer[bytesReceived] = '\0';
      Data data;
      data.data = strdup(buffer);
      networkContext.callback.onClientData(data);
      free(data.data);
    }
    else if (bytesReceived == 0)
    {
      pthread_mutex_unlock(&networkContext.lock);
      break;
    }
    else
    {
      strncpy(networkContext.lastError, "Recv failed from client\n", sizeof(networkContext.lastError) - 1);
      networkContext.lastError[sizeof(networkContext.lastError) - 1] = '\0';
      pthread_mutex_unlock(&networkContext.lock);
      break;
    }
    pthread_mutex_unlock(&networkContext.lock);
  }

  pthread_mutex_lock(&networkContext.lock);
  free(args);
  closeSocket(client->socket.socket);
  client->active = false;
  removeClient(clientIndex);
  pthread_mutex_unlock(&networkContext.lock);
  return NULL;
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