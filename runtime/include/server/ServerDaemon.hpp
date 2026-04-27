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
    /// Description: Describes the server daemon operation contract.
    explicit ServerDaemon(SimulationServer& server, std::string authToken = {});
    /// Description: Releases resources owned by ServerDaemon.
    ~ServerDaemon();
    /// Description: Describes the start operation contract.
    bool start(std::uint16_t port, const std::string& bindAddress = "127.0.0.1");
    /// Description: Describes the stop operation contract.
    void stop();
    /// Description: Describes the is running operation contract.
    bool isRunning() const;
    /// Description: Describes the shutdown requested operation contract.
    bool shutdownRequested() const;

private:
    typedef std::intptr_t SocketHandle;
    /// Description: Describes the accept loop operation contract.
    void acceptLoop();
    /// Description: Describes the handle client operation contract.
    void handleClient(SocketHandle client);
    /// Description: Describes the process request operation contract.
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
