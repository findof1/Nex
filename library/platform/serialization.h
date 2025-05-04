#ifndef SERIALIZATION_H
#define SERIALIZATION_H
#ifdef __cplusplus
extern "C"
{
#endif

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
  } Data;

  int sendInt(socket_t socket, int value);
  int recvInt(socket_t socket, int *out);

  int sendFloat(socket_t socket, float value);
  int recvFloat(socket_t socket, float *out);

  int sendString(socket_t socket, const char *str);
  int recvString(socket_t socket, char **out);

  int sendJSON(socket_t socket, const cJSON *json);
  int recvJSON(socket_t socket, cJSON **json);

  int recvAny(socket_t socket, Data *data);

  int sendIntTo(socket_t socket, struct sockaddr_in *peerAddr, int value);
  int recvIntFrom(socket_t socket, struct sockaddr_in *peerAddr, int *out);

  int sendFloatTo(socket_t socket, struct sockaddr_in *peerAddr, float value);
  int recvFloatFrom(socket_t socket, struct sockaddr_in *peerAddr, float *out);

  int sendStringTo(socket_t socket, struct sockaddr_in *peerAddr, const char *str);
  int recvStringFrom(socket_t socket, struct sockaddr_in *peerAddr, char **out);

  int sendJSONTo(socket_t socket, struct sockaddr_in *peerAddr, const cJSON *json);
  int recvJSONFrom(socket_t socket, struct sockaddr_in *peerAddr, cJSON **json);

  int recvAnyFrom(socket_t socket, struct sockaddr_in *peerAddr, Data *data);

  void freeRecvData(Data *data);

#ifdef __cplusplus
}
#endif
#endif