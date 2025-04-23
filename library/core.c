#include "core.h"
#include "internal.h"

static void *serverAcceptLoop(void *arg);
static void *clientDataLoop(void *arg);
static void *clientAcceptLoop(void *arg);

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

int startServer(int port, int maxClients, void (*onClientData)(RecvData))
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
    closeSocket(networkContext.socket.socket);
    return NETWORK_ERR_SOCKET;
  }

  networkContext.socket.addr = createSockaddrIn(8080, "0.0.0.0");
  if (bindSocket(networkContext.socket.socket, (struct sockaddr *)&networkContext.socket.addr, sizeof(networkContext.socket.addr)) == PLATFORM_FAILURE)
  {
    strncpy(networkContext.lastError, "Socket bind failed\n", sizeof(networkContext.lastError) - 1);
    networkContext.lastError[sizeof(networkContext.lastError) - 1] = '\0';
    closeSocket(networkContext.socket.socket);
    return NETWORK_ERR_BIND;
  }

  if (listenSocket(networkContext.socket.socket, 1) == PLATFORM_FAILURE)
  {
    strncpy(networkContext.lastError, "Listen failed\n", sizeof(networkContext.lastError) - 1);
    networkContext.lastError[sizeof(networkContext.lastError) - 1] = '\0';
    closeSocket(networkContext.socket.socket);
    return NETWORK_ERR_LISTEN;
  }

  if (pthread_create(&networkContext.server.acceptThread, NULL, serverAcceptLoop, NULL) != 0)
  {
    strncpy(networkContext.lastError, "pthread_create acceptLoop failed", sizeof(networkContext.lastError) - 1);
    networkContext.lastError[sizeof(networkContext.lastError) - 1] = '\0';
    closeSocket(networkContext.socket.socket);
    return NETWORK_ERR_THREAD;
  }

  pthread_detach(networkContext.server.acceptThread);
  networkContext.server.listening = true;

  return NETWORK_OK;
}

typedef struct
{
  ServerClient *client;
  int clientIndex;
} ClientThreadArgs;

static void *serverAcceptLoop(void *arg)
{
  while (networkContext.server.listening)
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
    if (clientSocket.socket < 0 && networkContext.server.listening)
    {
      strncpy(networkContext.lastError, "Client accept failed\n", sizeof(networkContext.lastError) - 1);
      networkContext.lastError[sizeof(networkContext.lastError) - 1] = '\0';
      continue;
    }

    if (!networkContext.server.listening)
    {
      break;
    }

    pthread_mutex_lock(&networkContext.lock);

    ServerClient *client = &networkContext.server.clients[clientIndex];
    client->socket = clientSocket;
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
      removeClient(clientIndex);
      free(args);
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
  RecvData clientAcceptedData;
  clientAcceptedData.type = TYPE_CONNECTED;
  networkContext.callback.onClientData(clientAcceptedData);

  ClientThreadArgs *args = (ClientThreadArgs *)arg;
  ServerClient *client = args->client;
  int clientIndex = args->clientIndex;

  while (networkContext.server.listening)
  {
    RecvData data;
    int result = recvAny(client->socket.socket, &data);
    pthread_mutex_lock(&networkContext.lock);

    if (result == PLATFORM_SUCCESS)
    {
      pthread_mutex_unlock(&networkContext.lock);
      networkContext.callback.onClientData(data);
      freeRecvData(&data);
    }
    else if (result == PLATFORM_CONNECTION_CLOSED)
    {
      pthread_mutex_unlock(&networkContext.lock);
      break;
    }
    else
    {
      pthread_mutex_unlock(&networkContext.lock);
    }
  }

  clientAcceptedData.type = TYPE_DISCONNECTED;
  networkContext.callback.onClientData(clientAcceptedData);

  pthread_mutex_lock(&networkContext.lock);

  free(args);
  pthread_mutex_unlock(&networkContext.lock);

  removeClient(clientIndex);
  return NULL;
}

int connectToServer(const char *ip, int port, void (*onServerData)(RecvData))
{
  if (onServerData == NULL)
  {
    strncpy(networkContext.lastError, "Invalid onServerData function: int startServer(int port, int maxClients, void (*onClientData)(Socket *, const char *, size_t))", sizeof(networkContext.lastError) - 1);
    networkContext.lastError[sizeof(networkContext.lastError) - 1] = '\0';
    return NETWORK_ERR_INVALID;
  }

  if (!networkContext.initialized)
  {
    strncpy(networkContext.lastError, "Platform Is Not Initialized!", sizeof(networkContext.lastError) - 1);
    networkContext.lastError[sizeof(networkContext.lastError) - 1] = '\0';
    return NETWORK_ERR_INVALID;
  }

  networkContext.callback.onServerData = onServerData;

  networkContext.socket.socket = createSocket(SOCK_STREAM, IPPROTO_TCP);
  if (networkContext.socket.socket == PLATFORM_FAILURE)
  {
    strncpy(networkContext.lastError, "Socket creation failed\n", sizeof(networkContext.lastError) - 1);
    networkContext.lastError[sizeof(networkContext.lastError) - 1] = '\0';
    return NETWORK_ERR_SOCKET;
  }

  networkContext.socket.addr = createSockaddrIn(port, ip);
  if (connectSocket(networkContext.socket.socket, (struct sockaddr *)&networkContext.socket.addr, sizeof(networkContext.socket.addr)) == PLATFORM_FAILURE)
  {
    strncpy(networkContext.lastError, "Failed to connect to server.\n", sizeof(networkContext.lastError) - 1);
    networkContext.lastError[sizeof(networkContext.lastError) - 1] = '\0';
    closeSocket(networkContext.socket.socket);
    return NETWORK_ERR_CONNECT;
  }

  if (pthread_create(&networkContext.client.serverThread, NULL, clientAcceptLoop, NULL) != 0)
  {
    strncpy(networkContext.lastError, "pthread_create acceptLoop failed", sizeof(networkContext.lastError) - 1);
    networkContext.lastError[sizeof(networkContext.lastError) - 1] = '\0';
    closeSocket(networkContext.socket.socket);
    return NETWORK_ERR_THREAD;
  }
  else
  {
    pthread_detach(networkContext.client.serverThread);
  }
  networkContext.client.running = true;

  return NETWORK_OK;
}

static void *clientAcceptLoop(void *arg)
{
  RecvData serverConnectedData;
  serverConnectedData.type = TYPE_CONNECTED;
  networkContext.callback.onServerData(serverConnectedData);

  while (networkContext.client.running)
  {
    RecvData data;
    int result = recvAny(networkContext.socket.socket, &data);
    pthread_mutex_lock(&networkContext.lock);

    if (result == PLATFORM_SUCCESS)
    {
      networkContext.callback.onServerData(data);
      freeRecvData(&data);
    }
    else if (result == PLATFORM_CONNECTION_CLOSED && networkContext.client.running)
    {
      networkContext.client.running = false;
      closeSocket(networkContext.socket.socket);
    }

    pthread_mutex_unlock(&networkContext.lock);
  }

  serverConnectedData.type = TYPE_DISCONNECTED;
  networkContext.callback.onServerData(serverConnectedData);
  return NULL;
}

int shutdownNetwork()
{
  if (networkContext.socketType == Server)
  {
    networkContext.server.listening = false;
    removeAllClients();
    closeSocket(networkContext.socket.socket);
  }

  if (networkContext.socketType == Client)
  {
    networkContext.client.running = false;
    closeSocket(networkContext.socket.socket);
  }
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