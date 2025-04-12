#pragma once
#include <stdint.h>
#include "platform.h"

#define TYPE_INT 1
#define TYPE_FLOAT 2
#define TYPE_STRING 3

int send_int(socket_t socket, int value);
int recv_int(socket_t socket, int *out);

int send_float(socket_t socket, float value);
int recv_float(socket_t socket, float *out);

int send_string(socket_t socket, const char *str);
int recv_string(socket_t socket, char **out);