/*
 * @file runtime/server/core/SrvDaemon.cpp
 * @brief Runtime daemon transport lifecycle and command dispatch.
 */

#include "core/Daemon.hpp"

#include "platform/socket/PltSocket.hpp"
#include "protocol/PtcProtocol.hpp"
#include "protocol/codec/PtcJsonCodec.hpp"

#include <cstddef>
#include <iostream>
#include <string_view>
#include <utility>

static std::string serverDaemonError(std::string_view operation, std::string_view detail)
{
    return std::string("[ipc] ") + std::string(operation) + ": " + std::string(detail);
}

Daemon::Daemon(SimulationServer& server, std::string authToken)
    : _server(server),
      _running(false),
      _shutdownRequested(false),
      _acceptThread(),
      _listenSocket(bltzr_socket::invalidHandle()),
      _bindAddress(kDefaultLoopbackHost),
      _authToken(std::move(authToken)),
      _port(0),
      _networkInitialized(false),
      _socketMutex(),
      _clientThreads()
{
}

Daemon::~Daemon()
{
    stop();
}

bool Daemon::start(std::uint16_t port, const std::string& bindAddress)
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
        _acceptThread = std::thread(&Daemon::acceptLoop, this);
        return true;
    }
    catch (const std::exception& ex) {
        std::cerr << serverDaemonError("start", ex.what()) << "\n";
        stop();
        return false;
    }
    catch (...) {
        std::cerr << serverDaemonError("start", "non-standard exception") << "\n";
        stop();
        return false;
    }
}

void Daemon::stop()
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
        for (std::thread& thread : _clientThreads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        _clientThreads.clear();
        if (_networkInitialized) {
            bltzr_socket::shutdownSocketLayer();
            _networkInitialized = false;
        }
    }
    catch (const std::exception& ex) {
        std::cerr << serverDaemonError("stop", ex.what()) << "\n";
    }
    catch (...) {
        std::cerr << serverDaemonError("stop", "non-standard exception") << "\n";
    }
    _running.store(false);
    _listenSocket = bltzr_socket::invalidHandle();
    _clientThreads.clear();
    if (_networkInitialized) {
        try {
            bltzr_socket::shutdownSocketLayer();
        }
        catch (const std::exception& ex) {
            std::cerr << serverDaemonError("stop shutdownSocketLayer", ex.what()) << "\n";
        }
        catch (...) {
            std::cerr << serverDaemonError("stop shutdownSocketLayer", "non-standard exception")
                      << "\n";
        }
        _networkInitialized = false;
    }
}

bool Daemon::isRunning() const
{
    return _running.load();
}

bool Daemon::shutdownRequested() const
{
    return _shutdownRequested.load();
}

std::string Daemon::processRequest(const std::string& request)
{
    try {
        bltzr_protocol::CommandRequest envelope{};
        std::string parseError;
        if (!bltzr_protocol::JsonCodec::parseRequest(request, envelope, parseError)) {
            return bltzr_protocol::JsonCodec::makeErrorResponse("unknown", parseError);
        }
        if (!_authToken.empty() && envelope.token != _authToken) {
            return bltzr_protocol::JsonCodec::makeErrorResponse("auth", "unauthorized");
        }
        const std::string& command = envelope.cmd;
        const auto observation = processObservationCommand(request, command);
        if (observation.has_value()) {
            return *observation;
        }
        const auto lifecycle = processLifecycleCommand(request, command);
        if (lifecycle.has_value()) {
            return *lifecycle;
        }
        const auto core = processCoreConfigurationCommand(request, command);
        if (core.has_value()) {
            return *core;
        }
        const auto treePm = processTreePmCommand(request, command);
        if (treePm.has_value()) {
            return *treePm;
        }
        const auto physics = processPhysicsCommand(request, command);
        if (physics.has_value()) {
            return *physics;
        }
        const auto persistence = processPersistenceCommand(request, command);
        if (persistence.has_value()) {
            return *persistence;
        }
        return bltzr_protocol::JsonCodec::makeErrorResponse(command, "unknown command");
    }
    catch (const std::exception& ex) {
        return bltzr_protocol::JsonCodec::makeErrorResponse(
            "request", serverDaemonError("processRequest", ex.what()));
    }
    catch (...) {
        return bltzr_protocol::JsonCodec::makeErrorResponse(
            "request", serverDaemonError("processRequest", "non-standard exception"));
    }
}
