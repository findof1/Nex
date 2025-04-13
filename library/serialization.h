#pragma once
#include <stdint.h>
#include "platform.h"
#include "cJSON.h"

#define TYPE_INT 1
#define TYPE_FLOAT 2
#define TYPE_STRING 3
#define TYPE_JSON 4

int sendInt(socket_t socket, int value);
int recvInt(socket_t socket, int *out);

int sendFloat(socket_t socket, float value);
int recvFloat(socket_t socket, float *out);

int sendString(socket_t socket, const char *str);
int recvString(socket_t socket, char **out);

int sendJSON(socket_t socket, const cJSON *json);
int recvJSON(socket_t socket, cJSON **json);