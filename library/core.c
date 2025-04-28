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

int startServer(int port, int maxClients, void (*onClientData)(Data, socket_t))
{
  if (networkContext.socketType != Server)
  {
    strncpy(networkContext.lastError, "Must have socketType Server passed into init() in order to call startServer()", sizeof(networkContext.lastError) - 1);
    networkContext.lastError[sizeof(networkContext.lastError) - 1] = '\0';
    return NETWORK_ERR_INVALID;
  }

  if (networkContext.connectionType != CONNECTION_TCP)
  {
    strncpy(networkContext.lastError, "Must have connection TCP type set in order to call startServer()", sizeof(networkContext.lastError) - 1);
    networkContext.lastError[sizeof(networkContext.lastError) - 1] = '\0';
    return NETWORK_ERR_INVALID;
  }

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

  networkContext.server.clients = (ServerClient *)calloc(maxClients, sizeof(ServerClient));
  networkContext.server.clientThreads = (pthread_t *)calloc(maxClients, sizeof(pthread_t));
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
    client->context = NULL;
    client->contextDeleter = NULL;

    pthread_t thread;
    ClientThreadArgs *args = (ClientThreadArgs *)malloc(sizeof(ClientThreadArgs));
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

  return NULL;
}

static void *clientDataLoop(void *arg)
{
  ClientThreadArgs *args = (ClientThreadArgs *)arg;
  ServerClient *client = args->client;
  int clientIndex = args->clientIndex;

  Data clientAcceptedData;
  clientAcceptedData.type = TYPE_CONNECTED;
  pthread_mutex_lock(&networkContext.lock);
  networkContext.callback.onClientData(clientAcceptedData, args->client->socket.socket);
  pthread_mutex_unlock(&networkContext.lock);

  while (networkContext.server.listening)
  {
    Data data;

    int result = recvAny(client->socket.socket, &data);
    pthread_mutex_lock(&networkContext.lock);

    if (result == PLATFORM_SUCCESS)
    {
      networkContext.callback.onClientData(data, args->client->socket.socket);
      pthread_mutex_unlock(&networkContext.lock);
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

  pthread_mutex_lock(&networkContext.lock);
  free(args);
  pthread_mutex_unlock(&networkContext.lock);

  removeClient(clientIndex);

  clientAcceptedData.type = TYPE_DISCONNECTED;
  pthread_mutex_lock(&networkContext.lock);
  networkContext.callback.onClientData(clientAcceptedData, -1);
  pthread_mutex_unlock(&networkContext.lock);

  return NULL;
}

int connectToServer(const char *ip, int port, void (*onServerData)(Data))
{
  if (networkContext.socketType != Client)
  {
    strncpy(networkContext.lastError, "Must have socketType Client passed into init() in order to call connectToServer()", sizeof(networkContext.lastError) - 1);
    networkContext.lastError[sizeof(networkContext.lastError) - 1] = '\0';
    return NETWORK_ERR_INVALID;
  }

  if (networkContext.connectionType != CONNECTION_TCP)
  {
    strncpy(networkContext.lastError, "Must have connection TCP type set in order to call connectToServer()", sizeof(networkContext.lastError) - 1);
    networkContext.lastError[sizeof(networkContext.lastError) - 1] = '\0';
    return NETWORK_ERR_INVALID;
  }

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
  Data serverConnectedData;
  serverConnectedData.type = TYPE_CONNECTED;
  networkContext.callback.onServerData(serverConnectedData);

  while (networkContext.client.running)
  {
    Data data;
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

int sendToAllClients(Data data)
{
  if (networkContext.socketType != Server)
  {
    strncpy(networkContext.lastError, "Must have socketType Server passed into init() in order to call sendToAllClients()", sizeof(networkContext.lastError) - 1);
    networkContext.lastError[sizeof(networkContext.lastError) - 1] = '\0';
    return NETWORK_ERR_INVALID;
  }

  if (networkContext.connectionType != CONNECTION_TCP)
  {
    strncpy(networkContext.lastError, "Must have connection TCP type set in order to call sendToAllClients()", sizeof(networkContext.lastError) - 1);
    networkContext.lastError[sizeof(networkContext.lastError) - 1] = '\0';
    return NETWORK_ERR_INVALID;
  }

  int result = NETWORK_OK;
  for (int i = 0; i < networkContext.server.numClients; i++)
  {
    int currentResult = sendToClient(data, networkContext.server.clients[i].socket.socket);
    if (currentResult != NETWORK_OK)
    {
      strncpy(networkContext.lastError, "Sending to all clients failed!", sizeof(networkContext.lastError) - 1);
      networkContext.lastError[sizeof(networkContext.lastError) - 1] = '\0';
      result = currentResult;
    }
  }
  return result;
}

int broadcastToClients(Data data, socket_t sender)
{
  if (networkContext.socketType != Server)
  {
    strncpy(networkContext.lastError, "Must have socketType Server passed into init() in order to call broadcastToClients()", sizeof(networkContext.lastError) - 1);
    networkContext.lastError[sizeof(networkContext.lastError) - 1] = '\0';
    return NETWORK_ERR_INVALID;
  }

  if (networkContext.connectionType != CONNECTION_TCP)
  {
    strncpy(networkContext.lastError, "Must have connection TCP type set in order to call broadcastToClients()", sizeof(networkContext.lastError) - 1);
    networkContext.lastError[sizeof(networkContext.lastError) - 1] = '\0';
    return NETWORK_ERR_INVALID;
  }

  int result = NETWORK_OK;
  for (int i = 0; i < networkContext.server.numClients; i++)
  {
    if (networkContext.server.clients[i].socket.socket == sender)
    {
      continue;
    }

    int currentResult = sendToClient(data, networkContext.server.clients[i].socket.socket);
    if (currentResult != NETWORK_OK)
    {
      strncpy(networkContext.lastError, "Broadcasting to clients failed!", sizeof(networkContext.lastError) - 1);
      networkContext.lastError[sizeof(networkContext.lastError) - 1] = '\0';
      result = currentResult;
    }
  }
  return result;
}

int setClientContext(void *context, socket_t client, void (*deleter)(void *))
{
  if (networkContext.socketType != Server)
  {
    strncpy(networkContext.lastError, "Must have socketType Server passed into init() in order to call setClientContext()", sizeof(networkContext.lastError) - 1);
    networkContext.lastError[sizeof(networkContext.lastError) - 1] = '\0';
    return NETWORK_ERR_INVALID;
  }

  if (networkContext.connectionType != CONNECTION_TCP)
  {
    strncpy(networkContext.lastError, "Must have connection TCP type set in order to call setClientContext()", sizeof(networkContext.lastError) - 1);
    networkContext.lastError[sizeof(networkContext.lastError) - 1] = '\0';
    return NETWORK_ERR_INVALID;
  }

  if (client == -1)
  {
    strncpy(networkContext.lastError, "Cannot set conext of an invalid or closed client", sizeof(networkContext.lastError) - 1);
    networkContext.lastError[sizeof(networkContext.lastError) - 1] = '\0';
    return NETWORK_ERR_INVALID;
  }

  for (int i = 0; i < networkContext.server.numClients; i++)
  {
    if (networkContext.server.clients[i].socket.socket != client)
    {
      continue;
    }

    networkContext.server.clients[i].context = context;
    networkContext.server.clients[i].contextDeleter = deleter;

    return NETWORK_OK;
  }

  return NETWORK_ERR_UNKNOWN;
}

void *getClientContext(socket_t client)
{
  if (networkContext.socketType != Server)
  {
    strncpy(networkContext.lastError, "Must have socketType Server passed into init() in order to call getClientContext()", sizeof(networkContext.lastError) - 1);
    networkContext.lastError[sizeof(networkContext.lastError) - 1] = '\0';
    return NETWORK_ERR_INVALID;
  }

  if (networkContext.connectionType != CONNECTION_TCP)
  {
    strncpy(networkContext.lastError, "Must have connection TCP type set in order to call getClientContext()", sizeof(networkContext.lastError) - 1);
    networkContext.lastError[sizeof(networkContext.lastError) - 1] = '\0';
    return NETWORK_ERR_INVALID;
  }

  if (client == -1)
  {
    strncpy(networkContext.lastError, "Cannot set conext of an invalid or closed client", sizeof(networkContext.lastError) - 1);
    networkContext.lastError[sizeof(networkContext.lastError) - 1] = '\0';
    return NULL;
  }

  for (int i = 0; i < networkContext.server.numClients; i++)
  {
    if (networkContext.server.clients[i].socket.socket != client)
    {
      continue;
    }

    if (networkContext.server.clients[i].context == NULL)
    {
      strncpy(networkContext.lastError, "Warning: Getting NULL client context in getClientContext().", sizeof(networkContext.lastError) - 1);
      networkContext.lastError[sizeof(networkContext.lastError) - 1] = '\0';
    }

    return networkContext.server.clients[i].context;
  }

  strncpy(networkContext.lastError, "Client passed into getClientContext does not exist!", sizeof(networkContext.lastError) - 1);
  networkContext.lastError[sizeof(networkContext.lastError) - 1] = '\0';
  return NULL;
}

int sendToClient(Data data, socket_t client)
{
  if (networkContext.connectionType != CONNECTION_TCP)
  {
    strncpy(networkContext.lastError, "Must have connection TCP type set in order to call sendToClient()", sizeof(networkContext.lastError) - 1);
    networkContext.lastError[sizeof(networkContext.lastError) - 1] = '\0';
    return NETWORK_ERR_INVALID;
  }

  int result = 0;
  switch (data.type)
  {
  case TYPE_INT:
    result = sendInt(client, data.data.i);
    break;
  case TYPE_FLOAT:
    result = sendFloat(client, data.data.f);
    break;
  case TYPE_STRING:
    result = sendString(client, data.data.s);
    break;
  case TYPE_JSON:
    result = sendJSON(client, data.data.json);
    break;
  }

  if (result == PLATFORM_FAILURE)
  {
    strncpy(networkContext.lastError, "Failed to send data to client", sizeof(networkContext.lastError) - 1);
    networkContext.lastError[sizeof(networkContext.lastError) - 1] = '\0';
    return NETWORK_ERR_SEND;
  }
  if (result == 0)
  {
    strncpy(networkContext.lastError, "Invalid data type passed into sendToClient()", sizeof(networkContext.lastError) - 1);
    networkContext.lastError[sizeof(networkContext.lastError) - 1] = '\0';
    return NETWORK_ERR_INVALID;
  }
  return NETWORK_OK;
}

int sendToServer(Data data)
{
  if (networkContext.socketType != Client)
  {
    strncpy(networkContext.lastError, "Must have socketType Client passed into init() in order to call sendToServer()", sizeof(networkContext.lastError) - 1);
    networkContext.lastError[sizeof(networkContext.lastError) - 1] = '\0';
    return NETWORK_ERR_INVALID;
  }

  if (networkContext.connectionType != CONNECTION_TCP)
  {
    strncpy(networkContext.lastError, "Must have connection TCP type set in order to call sendToServer()", sizeof(networkContext.lastError) - 1);
    networkContext.lastError[sizeof(networkContext.lastError) - 1] = '\0';
    return NETWORK_ERR_INVALID;
  }

  int result = 0;
  switch (data.type)
  {
  case TYPE_INT:
    result = sendInt(networkContext.socket.socket, data.data.i);
    break;
  case TYPE_FLOAT:
    result = sendFloat(networkContext.socket.socket, data.data.f);
    break;
  case TYPE_STRING:
    result = sendString(networkContext.socket.socket, data.data.s);
    break;
  case TYPE_JSON:
    result = sendJSON(networkContext.socket.socket, data.data.json);
    break;
  }

  if (result == PLATFORM_FAILURE)
  {
    strncpy(networkContext.lastError, "Failed to send data to server", sizeof(networkContext.lastError) - 1);
    networkContext.lastError[sizeof(networkContext.lastError) - 1] = '\0';
    return NETWORK_ERR_SEND;
  }
  if (result == 0)
  {
    strncpy(networkContext.lastError, "Invalid data type passed into sendToServer()", sizeof(networkContext.lastError) - 1);
    networkContext.lastError[sizeof(networkContext.lastError) - 1] = '\0';
    return NETWORK_ERR_INVALID;
  }
  return NETWORK_OK;
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
  return NETWORK_OK;
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

const char *getLastError()
{
  if (strlen(networkContext.lastError) > 0)
  {
    return networkContext.lastError;
  }

  return "No error occurred.\n";
}