/*
 * @file runtime/src/server/daemon/lifecycle.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Lifecycle management: start, stop, and state queries.
 */

#include "server/ServerDaemon.hpp"
#include "platform/SocketPlatform.hpp"
#include <iostream>
#include <mutex>

/*
 * @brief Start the daemon on a specified port and address.
 * @param port Input value used by this contract.
 * @param bindAddress Input value used by this contract.
 * @return bool value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool ServerDaemon::start(std::uint16_t port, const std::string& bindAddress)
{
    try {
        if (_running.load()) {
            return true;
        }
        if (!bltzr_socket::initializeSocketLayer()) {
            std::cerr << "[ipc] failed to initialize socket layer\n";
            return false;
        }
        _networkInitialized = true;
        const bltzr_socket::Handle listenSocket = bltzr_socket::createTcpSocket();
        if (!bltzr_socket::isValid(listenSocket)) {
            std::cerr << "[ipc] failed to create socket\n";
            stop();
            return false;
        }
        bltzr_socket::setReuseAddress(listenSocket, true);
        if (!bltzr_socket::bindIpv4(listenSocket, bindAddress, port)) {
            std::cerr << "[ipc] bind failed on " << bindAddress << ":" << port << "\n";
            bltzr_socket::closeSocket(listenSocket);
            stop();
            return false;
        }
        if (!bltzr_socket::listenSocket(listenSocket, 8)) {
            std::cerr << "[ipc] listen failed\n";
            bltzr_socket::closeSocket(listenSocket);
            stop();
            return false;
        }
        {
            std::lock_guard<std::mutex> lock(_socketMutex);
            _listenSocket = listenSocket;
            _bindAddress = bindAddress;
            _port = port;
        }
        _shutdownRequested.store(false);
        _running.store(true);
        _acceptThread = std::thread(&ServerDaemon::acceptLoop, this);
        return true;
    }
    catch (const std::exception& ex) {
        std::cerr << "[ipc] start: " << ex.what() << "\n";
        stop();
        return false;
    }
    catch (...) {
        std::cerr << "[ipc] start: non-standard exception\n";
        stop();
        return false;
    }
}

/*
 * @brief Stop the daemon and clean up all resources.
 * @param None This contract does not take explicit parameters.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void ServerDaemon::stop()
{
    try {
        _running.store(false);
        bltzr_socket::Handle listenSocket;
        {
            std::lock_guard<std::mutex> lock(_socketMutex);
            listenSocket = _listenSocket;
            _listenSocket = bltzr_socket::invalidHandle();
        }
        bltzr_socket::closeSocket(listenSocket);
        if (_acceptThread.joinable()) {
            _acceptThread.join();
        }
        for (std::thread& thread : _clientThreads)
            if (thread.joinable()) {
                thread.join();
            }
        _clientThreads.clear();
        if (_networkInitialized) {
            bltzr_socket::shutdownSocketLayer();
            _networkInitialized = false;
        }
    }
    catch (const std::exception& ex) {
        std::cerr << "[ipc] stop: " << ex.what() << "\n";
    }
    catch (...) {
        std::cerr << "[ipc] stop: non-standard exception\n";
    }
    _running.store(false);
    _listenSocket = bltzr_socket::invalidHandle();
    _clientThreads.clear();
    if (_networkInitialized) {
        try {
            bltzr_socket::shutdownSocketLayer();
        }
        catch (const std::exception& ex) {
            std::cerr << "[ipc] stop shutdownSocketLayer: " << ex.what() << "\n";
        }
        catch (...) {
            std::cerr << "[ipc] stop shutdownSocketLayer: non-standard exception\n";
        }
        _networkInitialized = false;
    }
}

/*
 * @brief Check if the daemon is currently running.
 * @param None This contract does not take explicit parameters.
 * @return bool value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool ServerDaemon::isRunning() const
{
    return _running.load();
}

/*
 * @brief Check if a shutdown has been requested via the protocol.
 * @param None This contract does not take explicit parameters.
 * @return bool value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool ServerDaemon::shutdownRequested() const
{
    return _shutdownRequested.load();
}
