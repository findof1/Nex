#include <iostream>
#include <memory>
#include <cstring>
#include "platform.h"
#include "serialization.h"
#include "nex.h"

class ClientInfo
{
public:
  int messageCount;

  ClientInfo() : messageCount(0) {}
};

void handleClientData(Data data, socket_t sender)
{
  switch (data.type)
  {
  case TYPE_CONNECTED:
  {
    ClientInfo *info = new ClientInfo();

    if (setClientContext(info, sender, [](void *ptr)
                         { delete static_cast<ClientInfo *>(ptr); }) != NETWORK_OK)
    {
      std::cerr << "Failed to set client context\n";
    }
    else
    {
      std::cout << "Client " << sender << " connected. Context initialized.\n";
    }
    break;
  }

  case TYPE_DISCONNECTED:
  {
    std::cout << "Client disconnected.\n";
    break;
  }

  case TYPE_STRING:
  {
    ClientInfo *info = static_cast<ClientInfo *>(getClientContext(sender));
    if (!info)
    {
      std::cout << "Client " << sender << " sent a string but has no context!\n";
      break;
    }
    info->messageCount++;
    std::cout << "Client " << sender << " says: \"" << data.data.s << "\" (message #" << info->messageCount << ")\n";

    std::string message = "Client " + std::to_string(sender) + ": " + (data.data.s ? data.data.s : "");

    Data sendData;
    sendData.type = TYPE_STRING;
    sendData.data.s = strdup(message.c_str());
    broadcastToClients(sendData, sender);
    free(sendData.data.s);
    break;
  }

  default:
    std::cout << "Client " << sender << " sent unsupported type " << data.type << "\n";
    break;
  }
}

int main()
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

  std::cout << "Server running on port 8080. Press Enter to shut down.\n";
  std::cin.get();

  shutdownNetwork();
  return 0;
}
