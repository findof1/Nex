#pragma once
#include <stdint.h>
#include "platform.h"
#include "cJSON.h"

typedef enum
{
  TYPE_INT = 1,
  TYPE_FLOAT = 2,
  TYPE_STRING = 3,
  TYPE_JSON = 4,
  TYPE_CONNECTED = 5,
  TYPE_DISCONNECTED = 6
} NetworkedType;
int sendInt(socket_t socket, int value);
int recvInt(socket_t socket, int *out);

int sendFloat(socket_t socket, float value);
int recvFloat(socket_t socket, float *out);

int sendString(socket_t socket, const char *str);
int recvString(socket_t socket, char **out);

int sendJSON(socket_t socket, const cJSON *json);
int recvJSON(socket_t socket, cJSON **json);

typedef struct
{
  NetworkedType type;
  union
  {
    int i;
    float f;
    char *s;
    cJSON *json;
  } data;
} RecvData;

int recvAny(socket_t socket, RecvData *data);
void freeRecvData(RecvData *data);