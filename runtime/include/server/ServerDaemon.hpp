/*
 * @file runtime/include/server/ServerDaemon.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Runtime public interfaces for protocol, command, client, and FFI boundaries.
 */

#ifndef BLITZAR_RUNTIME_INCLUDE_SERVER_SERVERDAEMON_HPP_
#define BLITZAR_RUNTIME_INCLUDE_SERVER_SERVERDAEMON_HPP_
#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
/*
 * @brief Defines the simulation server type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
class SimulationServer;

/*
 * @brief Defines the server daemon type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
class ServerDaemon {
public:
    /*
     * @brief Documents the server daemon operation contract.
     * @param server Input value used by this contract.
     * @param authToken Input value used by this contract.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    explicit ServerDaemon(SimulationServer& server, std::string authToken = {});
    /*
     * @brief Documents the ~server daemon operation contract.
     * @param None This contract does not take explicit parameters.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    ~ServerDaemon();
    /*
     * @brief Documents the start operation contract.
     * @param port Input value used by this contract.
     * @param bindAddress Input value used by this contract.
     * @return bool value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    bool start(std::uint16_t port, const std::string& bindAddress = "127.0.0.1");
    /*
     * @brief Documents the stop operation contract.
     * @param None This contract does not take explicit parameters.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void stop();
    /*
     * @brief Documents the is running operation contract.
     * @param None This contract does not take explicit parameters.
     * @return bool value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    bool isRunning() const;
    /*
     * @brief Documents the shutdown requested operation contract.
     * @param None This contract does not take explicit parameters.
     * @return bool value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    bool shutdownRequested() const;

private:
    typedef std::intptr_t SocketHandle;
    /*
     * @brief Documents the accept loop operation contract.
     * @param None This contract does not take explicit parameters.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void acceptLoop();
    /*
     * @brief Documents the handle client operation contract.
     * @param client Input value used by this contract.
     * @return No return value.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
    void handleClient(SocketHandle client);
    /*
     * @brief Documents the process request operation contract.
     * @param request Input value used by this contract.
     * @return std::string value produced by this contract.
     * @note Keep side effects explicit and preserve deterministic behavior where callers depend on
     * it.
     */
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
#endif // BLITZAR_RUNTIME_INCLUDE_SERVER_SERVERDAEMON_HPP_
