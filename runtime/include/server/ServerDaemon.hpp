// File: runtime/include/server/ServerDaemon.hpp
// Purpose: Runtime integration surface for BLITZAR clients and protocols.

#ifndef GRAVITY_RUNTIME_INCLUDE_SERVER_SERVERDAEMON_HPP_
#define GRAVITY_RUNTIME_INCLUDE_SERVER_SERVERDAEMON_HPP_
#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
/// Description: Defines the SimulationServer data or behavior contract.
class SimulationServer;
/// Description: Defines the ServerDaemon data or behavior contract.
class ServerDaemon {
public:
    explicit ServerDaemon(SimulationServer& server, std::string authToken = {});
    /// Description: Releases resources owned by ServerDaemon.
    ~ServerDaemon();
    /// Description: Executes the start operation.
    bool start(std::uint16_t port, const std::string& bindAddress = "127.0.0.1");
    /// Description: Executes the stop operation.
    void stop();
    /// Description: Executes the isRunning operation.
    bool isRunning() const;
    /// Description: Executes the shutdownRequested operation.
    bool shutdownRequested() const;

private:
    typedef std::intptr_t SocketHandle;
    /// Description: Executes the acceptLoop operation.
    void acceptLoop();
    /// Description: Executes the handleClient operation.
    void handleClient(SocketHandle client);
    /// Description: Executes the processRequest operation.
    std::string processRequest(const std::string& request);
    SimulationServer& _server;
    std::atomic<bool> _running;
    std::atomic<bool> _shutdownRequested;
    std::thread _acceptThread;
    SocketHandle _listenSocket;
    std::string _bindAddress;
    std::string _authToken;
    std::uint16_t _port;
    bool _networkInitialized;
    std::mutex _socketMutex;
    std::vector<std::thread> _clientThreads;
};
#endif // GRAVITY_RUNTIME_INCLUDE_SERVER_SERVERDAEMON_HPP_
