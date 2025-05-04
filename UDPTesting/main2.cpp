#include <iostream>
#include <memory>
#include <cstring>
#include "platform.h"
#include "serialization.h"
#include "core.h"

class PeerInfo
{
public:
  int messageCount;

  PeerInfo() : messageCount(0) {}
};

int connectedPeer = -1;

void handlePeerData(Data data, int peer)
{
  switch (data.type)
  {
  case TYPE_CONNECTED:
  {
    PeerInfo *info = new PeerInfo();
    connectedPeer = peer;

    if (setPeerContext(info, peer, [](void *ptr)
                       { delete static_cast<PeerInfo *>(ptr); }) != NETWORK_OK)
    {
      std::cerr << "Failed to set peer context\n";
    }
    else
    {
      std::cout << "Peer " << peer << " connected. Context initialized.\n";
    }
    break;
  }

  case TYPE_DISCONNECTED:
  {
    std::cout << "Peer disconnected.\n";
    break;
  }

  case TYPE_STRING:
  {
    PeerInfo *info = static_cast<PeerInfo *>(getPeerContext(peer));
    if (!info)
    {
      std::cout << "Peer " << peer << " sent a string but has no context!\n";
      break;
    }
    info->messageCount++;
    std::cout << "Peer " << peer << " says: \"" << data.data.s << "\" (message #" << info->messageCount << ")\n";
    break;
  }

  default:
    std::cout << "Peer " << peer << " sent unsupported type " << data.type << "\n";
    break;
  }
}

int main()
{
  if (init(CONNECTION_UDP, Peer) != NETWORK_OK)
  {
    printLastError();
    return 1;
  }

  if (startPeer(8001, 10, handlePeerData) != NETWORK_OK)
  {
    printLastError();
    shutdownNetwork();
    return 1;
  }

  std::cin.get();

  if (connectToPeer("127.0.0.1", 8000) != NETWORK_OK)
  {
    printLastError();
    shutdownNetwork();
    return 1;
  }

  char input[512];
  while (fgets(input, sizeof(input), stdin))
  {
    size_t len = strlen(input);
    if (len > 0 && input[len - 1] == '\n')
    {
      input[len - 1] = '\0';
      len--;
    }

    if (strcmp(input, "/quit") == 0)
    {
      break;
    }

    char *msg_str = strdup(input);
    if (!msg_str)
    {
      fprintf(stderr, "Out of memory!\n");
      break;
    }

    Data msg;
    msg.type = TYPE_STRING;
    msg.data.s = msg_str;

    if (connectedPeer == -1)
    {
      free(msg_str);
      continue;
    }

    if (sendToPeer(msg, connectedPeer) != NETWORK_OK)
    {
      printLastError();
      free(msg_str);
      break;
    }

    free(msg_str);
  }

  std::cin.get();

  shutdownNetwork();
  return 0;
}
