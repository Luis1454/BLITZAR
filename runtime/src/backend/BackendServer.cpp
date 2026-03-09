#include "backend/BackendServer.hpp"
#include "platform/SocketPlatform.hpp"

#include <iostream>
#include <string>
#include <string_view>
#include <utility>

static std::string backendServerError(std::string_view operation, std::string_view detail)
{
    return std::string("[ipc] ") + std::string(operation) + ": " + std::string(detail);
}

BackendServer::BackendServer(SimulationBackend &backend, std::string authToken)
    : _backend(backend),
      _running(false),
      _shutdownRequested(false),
      _acceptThread(),
      _listenSocket(grav_socket::invalidHandle()),
      _bindAddress("127.0.0.1"),
      _authToken(std::move(authToken)),
      _port(0),
      _networkInitialized(false),
      _socketMutex(),
      _clientThreads()
{
}

BackendServer::~BackendServer()
{
    stop();
}

bool BackendServer::start(std::uint16_t port, const std::string &bindAddress)
{
    try {
        if (_running.load()) {
            return true;
        }

        if (!grav_socket::initializeSocketLayer()) {
            std::cerr << "[ipc] failed to initialize socket layer\n";
            return false;
        }
        _networkInitialized = true;

        const grav_socket::Handle listenSocket = grav_socket::createTcpSocket();
        if (!grav_socket::isValid(listenSocket)) {
            std::cerr << "[ipc] failed to create socket\n";
            stop();
            return false;
        }

        grav_socket::setReuseAddress(listenSocket, true);

        if (!grav_socket::bindIpv4(listenSocket, bindAddress, port)) {
            std::cerr << "[ipc] bind failed on " << bindAddress << ":" << port << "\n";
            grav_socket::closeSocket(listenSocket);
            stop();
            return false;
        }

        if (!grav_socket::listenSocket(listenSocket, 8)) {
            std::cerr << "[ipc] listen failed\n";
            grav_socket::closeSocket(listenSocket);
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
        _acceptThread = std::thread(&BackendServer::acceptLoop, this);
        return true;
    } catch (const std::exception &ex) {
        std::cerr << backendServerError("start", ex.what()) << "\n";
        stop();
        return false;
    } catch (...) {
        std::cerr << backendServerError("start", "non-standard exception") << "\n";
        stop();
        return false;
    }
}

void BackendServer::stop()
{
    try {
        _running.store(false);

        grav_socket::Handle listenSocket = grav_socket::invalidHandle();
        {
            std::lock_guard<std::mutex> lock(_socketMutex);
            listenSocket = _listenSocket;
            _listenSocket = grav_socket::invalidHandle();
        }
        grav_socket::closeSocket(listenSocket);

        if (_acceptThread.joinable()) {
            _acceptThread.join();
        }
        for (std::thread &thread : _clientThreads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        _clientThreads.clear();

        if (_networkInitialized) {
            grav_socket::shutdownSocketLayer();
            _networkInitialized = false;
        }
    } catch (const std::exception &ex) {
        std::cerr << backendServerError("stop", ex.what()) << "\n";
    } catch (...) {
        std::cerr << backendServerError("stop", "non-standard exception") << "\n";
    }
    _running.store(false);
    _listenSocket = grav_socket::invalidHandle();
    _clientThreads.clear();
    if (_networkInitialized) {
        try {
            grav_socket::shutdownSocketLayer();
        } catch (const std::exception &ex) {
            std::cerr << backendServerError("stop shutdownSocketLayer", ex.what()) << "\n";
        } catch (...) {
            std::cerr << backendServerError("stop shutdownSocketLayer", "non-standard exception") << "\n";
        }
        _networkInitialized = false;
    }
}

bool BackendServer::isRunning() const
{
    return _running.load();
}

bool BackendServer::shutdownRequested() const
{
    return _shutdownRequested.load();
}


