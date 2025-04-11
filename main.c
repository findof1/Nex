#include <stdio.h>
#include "platform.h"

int main()
{
  if (platformInit() == PLATFORM_FAILURE)
  {
    printf("Failed to init. Exiting...");
    return 1;
  }

  socket_t socket = createSocket(SOCK_STREAM, IPPROTO_TCP);

  if (socket == PLATFORM_FAILURE)
  {
    printf("Failed to create socket. Exiting...");
    return 1;
  }

  struct sockaddr_in addr = createSockaddrIn(8080, "0.0.0.0");
  if (bindSocket(socket, (struct sockaddr *)&addr, (socklen_t)sizeof(addr)) == PLATFORM_FAILURE)
  {
    printf("Failed to bind socket. Exiting...");
    return 1;
  }

  if (listenSocket(socket, 10) == PLATFORM_FAILURE)
  {
    printf("Failed to listen. Exiting...");
    return 1;
  }

  printf("Listening on port 8080...");

  struct sockaddr_in clientAddr;
  int clientLen = sizeof(clientAddr);

  while (1)
  {
    socket_t clientSocket = accept(socket, (struct sockaddr *)&clientAddr, &clientLen);
    if (clientSocket == PLATFORM_FAILURE)
    {
      printf("Accept failed\n");
      continue;
    }

    char msg[] = "Hello World!";

    sendData(clientSocket, msg, sizeof(msg) - 1, 0);

    closeSocket(clientSocket);
  }

  platformCleanup();
  return 0;
}