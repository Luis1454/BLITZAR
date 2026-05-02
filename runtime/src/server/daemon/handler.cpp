/*
 * @file runtime/src/server/daemon/handler.cpp
 * @brief Client connection handler implementation.
 */

#include "server/ServerDaemon.hpp"
#include "server/daemon/helpers.hpp"
#include "platform/SocketPlatform.hpp"
#include <array>
#include <iostream>
#include <string>

void ServerDaemon::handleClient(SocketHandle client)
{
    try {
        const bltzr_socket::Handle clientSocket = client;
        std::string buffer;
        buffer.reserve(2048);
        std::array<char, 2048> chunk{};

        while (_running.load()) {
            const int received = bltzr_socket::recvBytes(
                clientSocket, bltzr_socket::MutableBytes{reinterpret_cast<std::byte*>(chunk.data()),
                                                         chunk.size()});
            if (received <= 0) {
                if (received < 0 && bltzr_socket::wouldBlockOrTimeoutLastError() &&
                    _running.load()) {
                    continue;
                }
                break;
            }
            buffer.append(chunk.data(), static_cast<std::size_t>(received));
            std::size_t newline = buffer.find('\n');
            while (newline != std::string::npos) {
                std::string request = daemonTrim(buffer.substr(0, newline));
                buffer.erase(0, newline + 1);
                if (!request.empty() && request.back() == '\r') {
                    request.pop_back();
                }
                if (request.empty()) {
                    continue;
                }
                const std::string response = processRequest(request) + "\n";
                if (!daemonSendAll(clientSocket, daemonAsBytes(response))) {
                    bltzr_socket::closeSocket(clientSocket);
                    return;
                }
                if (_shutdownRequested.load()) {
                    bltzr_socket::closeSocket(clientSocket);
                    return;
                }
                newline = buffer.find('\n');
            }
        }
        bltzr_socket::closeSocket(clientSocket);
    }
    catch (const std::exception& ex) {
        std::cerr << daemonServerError("handleClient", ex.what()) << "\n";
        bltzr_socket::closeSocket(static_cast<bltzr_socket::Handle>(client));
    }
    catch (...) {
        std::cerr << daemonServerError("handleClient", "non-standard exception") << "\n";
        bltzr_socket::closeSocket(static_cast<bltzr_socket::Handle>(client));
    }
}
