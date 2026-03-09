#include "backend/BackendServerLoop.hpp"
#include "platform/SocketPlatform.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <exception>
#include <iostream>
#include <string>
#include <string_view>

static std::string trim(const std::string &input)
{
    const auto begin = std::find_if_not(input.begin(), input.end(), [](unsigned char c) {
        return std::isspace(c) != 0;
    });
    const auto end = std::find_if_not(input.rbegin(), input.rend(), [](unsigned char c) {
        return std::isspace(c) != 0;
    }).base();
    if (begin >= end) {
        return {};
    }
    return std::string(begin, end);
}

static grav_socket::ConstBytes asBytes(std::string_view text)
{
    return grav_socket::ConstBytes{
        reinterpret_cast<const std::byte *>(text.data()),
        text.size()};
}

static bool sendAll(grav_socket::Handle socketHandle, grav_socket::ConstBytes bytes)
{
    std::size_t offset = 0;
    while (offset < bytes.size) {
        const int sent = grav_socket::sendBytes(socketHandle, bytes.subview(offset));
        if (sent <= 0) {
            return false;
        }
        offset += static_cast<std::size_t>(sent);
    }
    return true;
}

static std::string backendServerError(std::string_view operation, std::string_view detail)
{
    return std::string("[ipc] ") + std::string(operation) + ": " + std::string(detail);
}

void BackendServer::acceptLoop()
{
    try {
        while (_running.load()) {
            grav_socket::Handle listenSocket = grav_socket::invalidHandle();
            {
                std::lock_guard<std::mutex> lock(_socketMutex);
                listenSocket = _listenSocket;
            }
            if (!grav_socket::isValid(listenSocket)) {
                break;
            }
            if (!grav_socket::waitReadable(listenSocket, 200)) {
                continue;
            }

            const grav_socket::Handle clientSocket = grav_socket::acceptSocket(listenSocket);
            if (!grav_socket::isValid(clientSocket)) {
                continue;
            }

            grav_socket::setSocketTimeoutMs(clientSocket, 500);
            _clientThreads.emplace_back(&BackendServer::handleClient, this, clientSocket);
        }
    } catch (const std::exception &ex) {
        std::cerr << backendServerError("acceptLoop", ex.what()) << "\n";
        _running.store(false);
    } catch (...) {
        std::cerr << backendServerError("acceptLoop", "non-standard exception") << "\n";
        _running.store(false);
    }
}

void BackendServer::handleClient(SocketHandle client)
{
    try {
        const grav_socket::Handle clientSocket = client;
        std::string buffer;
        buffer.reserve(2048);
        std::array<char, 2048> chunk{};
        while (_running.load()) {
            const int received = grav_socket::recvBytes(
                clientSocket,
                grav_socket::MutableBytes{
                    reinterpret_cast<std::byte *>(chunk.data()),
                    chunk.size()});
            if (received <= 0) {
                if (received < 0 && grav_socket::wouldBlockOrTimeoutLastError() && _running.load()) {
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
                    continue;
                }

                const std::string response = processRequest(request) + "\n";
                if (!sendAll(clientSocket, asBytes(response)) || _shutdownRequested.load()) {
                    grav_socket::closeSocket(clientSocket);
                    return;
                }
                newline = buffer.find('\n');
            }
        }

        grav_socket::closeSocket(clientSocket);
    } catch (const std::exception &ex) {
        std::cerr << backendServerError("handleClient", ex.what()) << "\n";
        grav_socket::closeSocket(static_cast<grav_socket::Handle>(client));
    } catch (...) {
        std::cerr << backendServerError("handleClient", "non-standard exception") << "\n";
        grav_socket::closeSocket(static_cast<grav_socket::Handle>(client));
    }
}
