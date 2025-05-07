#ifndef NEX_H
#define NEX_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "platform.h"
#include "stdbool.h"
#include "serialization.h"

#ifdef NEX_EXPORTS
#define NEX_API __declspec(dllexport)
#else
#define NEX_API __declspec(dllimport)
#endif

  typedef enum
  {
    CONNECTION_TCP,
    CONNECTION_UDP
  } ConnectionType;

  typedef enum
  {
    NETWORK_OK = 0,
    NETWORK_ERR_SOCKET,
    NETWORK_ERR_BIND,
    NETWORK_ERR_LISTEN,
    NETWORK_ERR_ACCEPT,
    NETWORK_ERR_CONNECT,
    NETWORK_ERR_SEND,
    NETWORK_ERR_RECV,
    NETWORK_ERR_TIMEOUT,
    NETWORK_ERR_THREAD,
    NETWORK_ERR_INITIALIZATION,
    NETWORK_ERR_INVALID,
    NETWORK_ERR_MEMORY,
    NETWORK_ERR_UNKNOWN
  } NetworkError;

  typedef enum
  {
    Client,
    Server,
    Peer
  } SocketType;

  typedef struct
  {
    socket_t socket;
    struct sockaddr_in addr;
  } Socket;

  /// Initializes the library, must call before using other functions in the library.
  ///
  /// @param connectionType The socket framework you are using. Either 'CONNECTION_TCP' or 'CONNECTION_UDP'.
  /// @param socketType The type of socket. Either 'Server', 'Client', or 'Peer'.
  /// @return Returns 'NETWORK_OK' on success, else, an error code.
  NEX_API int init(ConnectionType connectionType, SocketType socketType);

  /// Starts a server socket and begins listening for clients.
  ///
  /// Must have called @ref init() with connectionType of CONNECTION_TCP to use.
  ///
  /// @param port The port to listen on.
  /// @param maxClients Maximum number of concurrent clients.
  /// @param onClientData Callback invoked when data is received from a client. The args must be of type Data and socket_t. socket_t will be the sender socket and can be used in functions such as @ref sendToClient()
  /// @return `NETWORK_OK` on success, else, an error code.
  /// @see init
  /// @see sendToClient
  NEX_API int startServer(int port, int maxClients, void (*onClientData)(Data, socket_t));

  /// Sends data to all clients connected to a server.
  ///
  /// Must have called @ref startServer() to use this function.
  ///
  /// @param data The data sent to the clients.
  /// @return `NETWORK_OK` on success, else, an error code.
  /// @see startServer
  NEX_API int sendToAllClients(Data data);

  /// Sends data to all clients connected to a server aside from a sender.
  ///
  /// An example use case is in a messaging app, where a client sends a message to the server,
  /// and the server broadcasts the message to all other clients, excluding the sender.
  ///
  /// Must have called @ref startServer() to use this function.
  ///
  /// @param data The data sent to the clients.
  /// @param sender The original sender that doesn't receive the data.
  /// @return `NETWORK_OK` on success, else, an error code.
  /// @see startServer
  NEX_API int broadcastToClients(Data data, socket_t sender);

  /// Sends data to a specific client connected to a server.
  ///
  /// Must have called @ref startServer() to use this function.
  ///
  /// @param data The data sent to the client.
  /// @param client The client you are sending the data to.
  /// @return `NETWORK_OK` on success, else, an error code.
  /// @see startServer
  NEX_API int sendToClient(Data data, socket_t client);

  /// Sets a data structure to be associated with a connected client.
  ///
  /// Must have called @ref startServer() to use this function.
  ///
  /// @param context A pointer pointing to the data structure you want associated with the client.
  /// @param client The client of which you want to set the context for.
  /// @param deleter A deleter function that properly frees the structure passed into context.
  /// @return `NETWORK_OK` on success, else, an error code.
  /// @see startServer
  NEX_API int setClientContext(void *context, socket_t client, void (*deleter)(void *));

  /// Gets the context set by @ref setClientContext() from a client.
  ///
  /// Must have called @ref startServer() to use this function.
  ///
  /// @param client The client of which you want to get the context from.
  /// @return A pointer to the context.
  /// @see setClientContext
  /// @see startServer
  NEX_API void *getClientContext(socket_t client);

  /// Starts a client socket and connects to a server.
  ///
  /// Must have called @ref init() with connectionType of CONNECTION_TCP to use.
  ///
  /// @param ip The IP address of the server.
  /// @param port The port the server is listening on.
  /// @param onServerData Callback invoked when data is received from the server.
  /// @return `NETWORK_OK` on success, else, an error code.
  /// @see init
  NEX_API int connectToServer(const char *ip, int port, void (*onServerData)(Data));

  /// Sends data to the server previously connected to.
  ///
  /// Must have called @ref connectToServer() to use this function.
  ///
  /// @param data The data sent to the server.
  /// @return `NETWORK_OK` on success, else, an error code.
  /// @see connectToServer
  NEX_API int sendToServer(Data data);

  /// Starts a peer socket to later connect with peers via @ref connectToPeer().
  ///
  /// Must have called @ref init() with connectionType of CONNECTION_UDP to use.
  ///
  /// @param port The port the peer is located on.
  /// @param maxPeers Maximum number of concurrent peers.
  /// @param onPeerData Callback invoked when data is received from another connected peer.
  /// @return `NETWORK_OK` on success, else, an error code.
  /// @see connectToPeer
  /// @see init
  NEX_API int startPeer(int port, int maxPeers, void (*onPeerData)(Data, int));

  /// Connects to another peer. When data gets recveived, it calls onPeerData that was passed into @ref startPeer().
  ///
  /// Must have called @ref startPeer() to use this function.
  ///
  /// @param ip The IP address of the peer. Use '127.0.0.1' for other peers on the same machine.
  /// @param port The port the other peer is located on.
  /// @return `NETWORK_OK` on success, else, an error code.
  /// @see startPeer
  NEX_API int connectToPeer(const char *ip, int port);

  /// Sends data to a specific connected peer.
  ///
  /// Must have called @ref connectToPeer() to use this function.
  ///
  /// @param data The data to send to the peer.
  /// @param peer The unique identifier sent to the peer. Note: These identifier are never reused, so it is safe to store a copy; however, they become invalid once a connection is closed.
  /// @return `NETWORK_OK` on success, else, an error code.
  /// @see connectToPeer
  NEX_API int sendToPeer(Data data, int peer);

  /// Sets a data structure to be associated with a connected peer.
  ///
  /// Must have called @ref connectToPeer() to use this function.
  ///
  /// @param context A pointer pointing to the data structure you want associated with the peer.
  /// @param peer The peer of which you want to set the context for.
  /// @param deleter A deleter function that properly frees the structure passed into context.
  /// @return `NETWORK_OK` on success, else, an error code.
  /// @see connectToPeer
  NEX_API int setPeerContext(void *context, int peer, void (*deleter)(void *));

  /// Gets the context set by @ref setPeerContext() from a peer.
  ///
  /// Must have called @ref connectToPeer() to use this function.
  ///
  /// @param peer The peer of which you want to get the context from.
  /// @return A pointer to the context.
  /// @see setPeerContext
  /// @see connectToPeer
  NEX_API void *getPeerContext(int peer);

  /// Prints out the last known error.
  NEX_API void printLastError();

  /// Gets the last known error and returns it.
  ///
  /// @return The error as a const char*
  NEX_API const char *getLastError();

  /// Shutdowns the network and cleans all network resources up.
  ///
  /// If you want to use library functions after this function has been called. @ref init() has to be called again.
  ///
  /// @return `NETWORK_OK` on success, else, an error code.
  /// @see init
  NEX_API int shutdownNetwork();

#ifdef __cplusplus
}
#endif

#endif