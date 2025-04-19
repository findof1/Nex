#include <stdio.h>
#include "platform.h"
#include "serialization.h"
#include <assert.h>
#include "core.h"

void handleClientData(RecvData data)
{
  switch (data.type)
  {
  case TYPE_CLIENT_ACCEPTED:
    printf("Client accepted!\n");
    break;
  case TYPE_INT:
    printf("Received int: %d\n", data.data.i);
    break;

  case TYPE_FLOAT:
    printf("Received float: %f\n", data.data.f);
    break;

  case TYPE_STRING:
    if (data.data.s)
      printf("Received string: %s\n", data.data.s);
    else
      printf("Received string: (null)\n");
    break;
  case TYPE_JSON:
  {
    if (data.data.json)
    {
      char *json_text = cJSON_Print(data.data.json);
      if (json_text)
      {
        printf("Received JSON: %s\n", json_text);
        free(json_text);
      }
      else
      {
        printf("Received JSON, but failed to print it\n");
      }
    }
    else
    {
      printf("Received JSON: (null)\n");
    }
    break;
  }
  default:
    printf("Received unknown type (%d)\n", (int)data.type);
    break;
  }
}

int main()
{
  if (init(CONNECTION_TCP, Server) != NETWORK_OK)
    printf("Error initializing");

  startServer(8080, 5, handleClientData);

  getchar();

  platformCleanup();
  return 0;
}