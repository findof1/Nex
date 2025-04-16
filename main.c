#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "platform.h"
#include "serialization.h"

void *handleClient(void *arg);

int main()
{
  if (platformInit() != PLATFORM_SUCCESS)
  {
    printf("Platform initialization failed\n");
    return 1;
  }

  socket_t server = createSocket(SOCK_STREAM, IPPROTO_TCP);
  struct sockaddr_in addr = createSockaddrIn(8080, "127.0.0.1");

  if (bindSocket(server, (struct sockaddr *)&addr, sizeof(addr)) != 0)
  {
    printf("Bind failed\n");
    return 1;
  }

  if (listenSocket(server, 10) != 0)
  {
    printf("Listen failed\n");
    return 1;
  }

  printf("Server is running on port 8080. Waiting for clients...\n");

  while (1)
  {
    struct sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    socket_t *client = malloc(sizeof(socket_t));
    *client = acceptSocket(server, (struct sockaddr *)&clientAddr, &addrLen);
    if (*client < 0)
    {
      printf("Accept failed\n");
      free(client);
      continue;
    }

    printf("[Server] Accepted new connection\n");

    pthread_t thread;
    int result = pthread_create(&thread, NULL, handleClient, client);
    if (result != 0)
    {
      printf("Failed to create thread\n");
      closeSocket(*client);
      free(client);
    }
    else
    {
      pthread_detach(thread);
    }
  }

  closeSocket(server);
  platformCleanup();
  return 0;
}

void *handleClient(void *arg)
{
  socket_t client = *(socket_t *)arg;
  free(arg);

  int intVal;
  if (recvInt(client, &intVal) == PLATFORM_SUCCESS)
    printf("[Client] Received int: %d\n", intVal);

  float floatVal;
  if (recvFloat(client, &floatVal) == PLATFORM_SUCCESS)
    printf("[Client] Received float: %f\n", floatVal);

  char *recvStr = NULL;
  if (recvString(client, &recvStr) == PLATFORM_SUCCESS)
  {
    printf("[Client] Received string: %s\n", recvStr);
    free(recvStr);
  }

  cJSON *recvJson = NULL;
  if (recvJSON(client, &recvJson) == PLATFORM_SUCCESS)
  {
    char *jsonStr = cJSON_Print(recvJson);
    printf("[Client] Received JSON:\n%s\n", jsonStr);
    free(jsonStr);
    cJSON_Delete(recvJson);
  }

  closeSocket(client);
  printf("[Client] Connection closed.\n");

  return NULL;
}