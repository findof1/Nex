#include <stdio.h>
#include "platform.h"
#include "serialization.h"
#include <assert.h>
#include "nex.h"

typedef struct
{
  int messageCount;
} ClientInfo;

void handleClientData(Data data, socket_t sender)
{
  switch (data.type)
  {
  case TYPE_CONNECTED:
  {
    ClientInfo *info = malloc(sizeof(ClientInfo));
    if (!info)
    {
      fprintf(stderr, "Failed to allocate ClientInfo\n");
      return;
    }
    info->messageCount = 0;

    if (setClientContext(info, sender, free) != NETWORK_OK)
    {
      fprintf(stderr, "Failed to set client context\n");
      free(info);
    }
    else
    {
      printf("Client %u connected. Context initialized.\n", (unsigned)sender);
    }
    break;
  }

  case TYPE_DISCONNECTED:
  {

    printf("Client disconnected.\n");

    break;
  }

  case TYPE_STRING:
  {
    ClientInfo *info = getClientContext(sender);
    if (!info)
    {
      printf("Client %u sent a string but has no context!\n", (unsigned)sender);
      break;
    }
    info->messageCount++;
    printf("Client %u says: \"%s\" (message #%d)\n", (unsigned)sender, data.data.s, info->messageCount);

    const char *orig = data.data.s ? data.data.s : "";
    size_t needed = snprintf(NULL, 0, "Client %u: %s", (unsigned)sender, orig) + 1;
    char *buf = malloc(needed);
    snprintf(buf, needed, "Client %u: %s", (unsigned)sender, orig);
    Data sendData;
    sendData.type = TYPE_STRING;
    sendData.data.s = buf;
    broadcastToClients(sendData, sender);
    printLastError();
    free(buf);
    break;
  }

  default:
    printf("Client %u sent unsupported type %d\n",
           (unsigned)sender, (int)data.type);
    break;
  }
}

int main(void)
{
  if (init(CONNECTION_TCP, Server) != NETWORK_OK)
  {
    printLastError();
    return 1;
  }

  if (startServer(8080, 10, handleClientData) != NETWORK_OK)
  {
    printLastError();
    shutdownNetwork();
    return 1;
  }

  printf("Server running on port 8080. Press Enter to shut down.\n");
  getchar();

  shutdownNetwork();
  return 0;
}