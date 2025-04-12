#include "serialization.h"

int send_int(socket_t socket, int value)
{
  uint8_t type = TYPE_INT;
  uint32_t size = htonl(sizeof(value));
  uint32_t number = htonl(value);
  if (sendData(socket, &type, sizeof(uint8_t), 0) == PLATFORM_FAILURE)
  {
    return PLATFORM_FAILURE;
  }
  if (sendData(socket, &size, sizeof(uint32_t), 0) == PLATFORM_FAILURE)
  {
    return PLATFORM_FAILURE;
  }
  if (sendData(socket, &number, sizeof(uint32_t), 0) == PLATFORM_FAILURE)
  {
    return PLATFORM_FAILURE;
  }
  return PLATFORM_SUCCESS;
}

int recv_int(socket_t socket, int *out)
{
  uint8_t type;
  uint32_t size, netValue;

  if (recvData(socket, &type, 1, 0) == PLATFORM_FAILURE)
  {
    return PLATFORM_FAILURE;
  }

  if (type != TYPE_INT)
  {
    return PLATFORM_FAILURE;
  }

  if (recvData(socket, &size, 4, 0) == PLATFORM_FAILURE)
  {
    return PLATFORM_FAILURE;
  }

  if (ntohl(size) != sizeof(uint32_t))
  {
    return PLATFORM_FAILURE;
  }

  if (recvData(socket, &netValue, 4, 0) == PLATFORM_FAILURE)
  {
    return PLATFORM_FAILURE;
  }

  *out = ntohl(netValue);
  return PLATFORM_SUCCESS;
}

int send_float(socket_t socket, float value)
{
  uint8_t type = TYPE_FLOAT;
  uint32_t size = htonl(sizeof(value));

  uint32_t number;
  memcpy(&number, &value, sizeof(float));
  number = htonl(number);

  if (sendData(socket, &type, sizeof(uint8_t), 0) == PLATFORM_FAILURE)
  {
    return PLATFORM_FAILURE;
  }
  if (sendData(socket, &size, sizeof(uint32_t), 0) == PLATFORM_FAILURE)
  {
    return PLATFORM_FAILURE;
  }
  if (sendData(socket, &number, sizeof(uint32_t), 0) == PLATFORM_FAILURE)
  {
    return PLATFORM_FAILURE;
  }
  return PLATFORM_SUCCESS;
}

int recv_float(socket_t socket, float *out)
{
  uint8_t type;
  uint32_t size, netValue;

  if (recvData(socket, &type, 1, 0) == PLATFORM_FAILURE)
  {
    return PLATFORM_FAILURE;
  }

  if (type != TYPE_FLOAT)
  {
    return PLATFORM_FAILURE;
  }

  if (recvData(socket, &size, 4, 0) == PLATFORM_FAILURE)
  {
    return PLATFORM_FAILURE;
  }

  if (ntohl(size) != sizeof(float))
  {
    return PLATFORM_FAILURE;
  }

  if (recvData(socket, &netValue, 4, 0) == PLATFORM_FAILURE)
  {
    return PLATFORM_FAILURE;
  }
  netValue = ntohl(netValue);
  float number;
  memcpy(out, &netValue, sizeof(uint32_t));

  return PLATFORM_SUCCESS;
}

int send_string(socket_t socket, const char *str)
{
  uint8_t type = TYPE_FLOAT;
  uint32_t length = strlen(str);
  uint32_t size = htonl(length);

  if (sendData(socket, &type, sizeof(uint8_t), 0) == PLATFORM_FAILURE)
  {
    return PLATFORM_FAILURE;
  }
  if (sendData(socket, &size, sizeof(uint32_t), 0) == PLATFORM_FAILURE)
  {
    return PLATFORM_FAILURE;
  }
  if (sendData(socket, &str, length, 0) == PLATFORM_FAILURE)
  {
    return PLATFORM_FAILURE;
  }
  return PLATFORM_SUCCESS;
}

int recv_string(socket_t socket, char **out)
{
  uint8_t type;
  uint32_t size;
  char *str;

  if (recvData(socket, &type, 1, 0) == PLATFORM_FAILURE)
  {
    return PLATFORM_FAILURE;
  }

  if (type != TYPE_STRING)
  {
    return PLATFORM_FAILURE;
  }

  if (recvData(socket, &size, 4, 0) == PLATFORM_FAILURE)
  {
    return PLATFORM_FAILURE;
  }
  size = ntohl(size);

  *out = (char *)malloc(size + 1);
  if (*out == NULL)
  {
    return PLATFORM_FAILURE;
  }

  if (recvData(socket, *out, size, 0) == PLATFORM_FAILURE)
  {
    free(*out);
    return PLATFORM_FAILURE;
  }

  (*out)[size] = '\0';

  return PLATFORM_SUCCESS;
}