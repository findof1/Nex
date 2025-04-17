#include <stdio.h>
#include "platform.h"
#include "serialization.h"
#include <assert.h>
#include "core.h"

void run_tests()
{
  init(CONNECTION_TCP, Server);

  socket_t server = createSocket(SOCK_STREAM, IPPROTO_TCP);
  struct sockaddr_in addr = createSockaddrIn(8080, "127.0.0.1");
  bindSocket(server, (struct sockaddr *)&addr, sizeof(addr));
  listenSocket(server, 1);

  socket_t client = createSocket(SOCK_STREAM, IPPROTO_TCP);
  connectSocket(client, (struct sockaddr *)&addr, sizeof(addr));

  struct sockaddr_in clientAddr;
  socklen_t addrLen = sizeof(clientAddr);
  socket_t accepted = acceptSocket(server, (struct sockaddr *)&clientAddr, &addrLen);

  int intVal = 42, intRecv;
  assert(sendInt(client, intVal) == PLATFORM_SUCCESS);
  assert(recvInt(accepted, &intRecv) == PLATFORM_SUCCESS);
  assert(intRecv == intVal);

  float floatVal = 3.14159f, floatRecv;
  assert(sendFloat(client, floatVal) == PLATFORM_SUCCESS);
  assert(recvFloat(accepted, &floatRecv) == PLATFORM_SUCCESS);
  assert(floatRecv == floatVal);

  const char *strVal = "Hello, socket!";
  char *strRecv = NULL;
  assert(sendString(client, strVal) == PLATFORM_SUCCESS);
  assert(recvString(accepted, &strRecv) == PLATFORM_SUCCESS);
  assert(strcmp(strVal, strRecv) == 0);
  free(strRecv);

  cJSON *json = cJSON_CreateObject();
  cJSON_AddStringToObject(json, "message", "hello");
  cJSON_AddNumberToObject(json, "value", 123);

  assert(sendJSON(client, json) == PLATFORM_SUCCESS);

  cJSON *jsonRecv = NULL;
  assert(recvJSON(accepted, &jsonRecv) == PLATFORM_SUCCESS);

  const char *msg = cJSON_GetObjectItem(jsonRecv, "message")->valuestring;
  int val = cJSON_GetObjectItem(jsonRecv, "value")->valueint;

  assert(strcmp(msg, "hello") == 0);
  assert(val == 123);

  cJSON_Delete(json);
  cJSON_Delete(jsonRecv);

  closeSocket(client);
  closeSocket(accepted);
  closeSocket(server);
  platformCleanup();

  printf("All tests passed.\n");
}

int main()
{
  run_tests();
  return 0;
}