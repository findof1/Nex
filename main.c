#include <stdio.h>
#include "platform.h"
#include "serialization.h"
#include <assert.h>
#include "core.h"

void handleClientData(Data data) { printf("Received from client: %s\n", data.data); }

int main()
{
  if (init(CONNECTION_TCP, Server) != NETWORK_OK)
    printf("Error initializing");

  startServer(8080, 5, handleClientData);

  getchar();

  platformCleanup();
  return 0;
}