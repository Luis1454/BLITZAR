/*
 * @file runtime/src/server/daemon/daemon.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Constructor and destructor for ServerDaemon.
 */

#include "server/ServerDaemon.hpp"
#include <utility>

/*
 * @brief Construct a ServerDaemon bound to a SimulationServer.
 * @param server Input value used by this contract.
 * @param authToken Input value used by this contract.
 * @return ServerDaemon instance initialized with all state.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
ServerDaemon::ServerDaemon(SimulationServer& server, std::string authToken)
    : _server(server),
      _running(false),
      _shutdownRequested(false),
      _acceptThread(),
      _listenSocket(bltzr_socket::invalidHandle()),
      _bindAddress("127.0.0.1"),
      _authToken(std::move(authToken)),
      _port(0),
      _networkInitialized(false),
      _socketMutex(),
      _clientThreads()
{
}

/*
 * @brief Destroy the ServerDaemon, ensuring clean shutdown.
 * @param None This contract does not take explicit parameters.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
ServerDaemon::~ServerDaemon()
{
    stop();
}
