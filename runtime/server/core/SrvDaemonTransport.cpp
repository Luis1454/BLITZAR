/*
 * @file runtime/server/core/SrvDaemonTransport.cpp
 * @brief Runtime daemon socket acceptance and client transport.
 */

#include "core/Daemon.hpp"

#include "PltSocket.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <iostream>
#include <string_view>

static std::string trim(std::string_view input)
{
    const auto begin = std::find_if_not(input.begin(), input.end(), [](unsigned char c) {
        return std::isspace(c) != 0;
    });
    const auto end = std::find_if_not(input.rbegin(), input.rend(), [](unsigned char c) {
                         return std::isspace(c) != 0;
                     }).base();
    return begin >= end ? std::string{} : std::string(begin, end);
}

static bltzr_socket::ConstBytes asBytes(std::string_view text)
{
    return {reinterpret_cast<const std::byte*>(text.data()), text.size()};
}

static bool sendAll(bltzr_socket::Handle socketHandle, bltzr_socket::ConstBytes bytes)
{
    std::size_t offset = 0;
    while (offset < bytes.size) {
        const int sent = bltzr_socket::sendBytes(socketHandle, bytes.subview(offset));
        if (sent <= 0) {
            return false;
        }
        offset += static_cast<std::size_t>(sent);
    }
    return true;
}

static std::string serverDaemonError(std::string_view operation, std::string_view detail)
{
    return std::string("[ipc] ") + std::string(operation) + ": " + std::string(detail);
}

void Daemon::acceptLoop()
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
            if (!bltzr_socket::waitReadable(listenSocket, kServicePollIntervalMs)) {
                continue;
            }
            const bltzr_socket::Handle clientSocket = bltzr_socket::acceptSocket(listenSocket);
            if (!bltzr_socket::isValid(clientSocket)) {
                continue;
            }
            bltzr_socket::setSocketTimeoutMs(clientSocket, kDaemonClientSocketTimeoutMs);
            _clientThreads.emplace_back(&Daemon::handleClient, this, clientSocket);
        }
    }
    catch (const std::exception& ex) {
        std::cerr << serverDaemonError("acceptLoop", ex.what()) << "\n";
        _running.store(false);
    }
    catch (...) {
        std::cerr << serverDaemonError("acceptLoop", "non-standard exception") << "\n";
        _running.store(false);
    }
}

void Daemon::handleClient(SocketHandle client)
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
                std::string request = trim(buffer.substr(0, newline));
                buffer.erase(0, newline + 1);
                if (!request.empty() && request.back() == '\r') {
                    request.pop_back();
                }
                if (request.empty()) {
                    newline = buffer.find('\n');
                    continue;
                }
                const std::string response = processRequest(request) + "\n";
                if (!sendAll(clientSocket, asBytes(response))) {
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
        std::cerr << serverDaemonError("handleClient", ex.what()) << "\n";
        bltzr_socket::closeSocket(static_cast<bltzr_socket::Handle>(client));
    }
    catch (...) {
        std::cerr << serverDaemonError("handleClient", "non-standard exception") << "\n";
        bltzr_socket::closeSocket(static_cast<bltzr_socket::Handle>(client));
    }
}
