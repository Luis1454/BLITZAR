/*
 * @file runtime/src/server/daemon/acceptance.cpp
 * @brief Accept loop implementation extracted from protocol logic.
 */

#include "server/ServerDaemon.hpp"
#include "server/daemon/helpers.hpp"
#include "platform/SocketPlatform.hpp"
#include <iostream>
#include <mutex>

void ServerDaemon::acceptLoop()
{
    try {
        while (_running.load()) {
            bltzr_socket::Handle listenSocket;
            {
                std::lock_guard<std::mutex> lock(_socketMutex);
                listenSocket = _listenSocket;
            }
            if (!bltzr_socket::isValid(listenSocket)) {
                break;
            }
            if (!bltzr_socket::waitReadable(listenSocket, 200)) {
                continue;
            }
            const bltzr_socket::Handle clientSocket = bltzr_socket::acceptSocket(listenSocket);
            if (!bltzr_socket::isValid(clientSocket)) {
                continue;
            }
            bltzr_socket::setSocketTimeoutMs(clientSocket, 500);
            _clientThreads.emplace_back(&ServerDaemon::handleClient, this, clientSocket);
        }
    }
    catch (const std::exception& ex) {
        std::cerr << daemonServerError("acceptLoop", ex.what()) << "\n";
        _running.store(false);
    }
    catch (...) {
        std::cerr << daemonServerError("acceptLoop", "non-standard exception") << "\n";
        _running.store(false);
    }
}
