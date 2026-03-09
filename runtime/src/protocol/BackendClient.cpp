#include "protocol/BackendClient.hpp"
#include "platform/SocketPlatform.hpp"

#include <cstddef>
#include <exception>
#include <iostream>
#include <string>
#include <string_view>

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
        const int sent = grav_socket::sendBytes(
            socketHandle,
            bytes.subview(offset));
        if (sent <= 0) {
            return false;
        }
        offset += static_cast<std::size_t>(sent);
    }
    return true;
}

static std::string backendClientError(std::string_view operation, std::string_view detail)
{
    return std::string("[backend-client] ") + std::string(operation) + ": " + std::string(detail);
}

BackendClient::BackendClient()
    : _socket(grav_socket::invalidHandle()),
      _socketTimeoutMs(3000),
      _networkInitialized(false),
      _recvBuffer(),
      _authToken()
{
}

BackendClient::~BackendClient()
{
    disconnect();
}

bool BackendClient::connect(const std::string &host, std::uint16_t port)
{
    try {
        disconnect();

        if (!grav_socket::initializeSocketLayer()) {
            return false;
        }
        _networkInitialized = true;

        const grav_socket::Handle socketHandle = grav_socket::createTcpSocket();
        if (!grav_socket::isValid(socketHandle)) {
            disconnect();
            return false;
        }

        if (!grav_socket::connectIpv4(socketHandle, host, port, _socketTimeoutMs)) {
            grav_socket::closeSocket(socketHandle);
            disconnect();
            return false;
        }

        grav_socket::setSocketTimeoutMs(socketHandle, _socketTimeoutMs);

        _socket = socketHandle;
        _recvBuffer.clear();
        return true;
    } catch (const std::exception &ex) {
        std::cerr << backendClientError("connect", ex.what()) << "\n";
        disconnect();
        return false;
    } catch (...) {
        std::cerr << backendClientError("connect", "non-standard exception") << "\n";
        disconnect();
        return false;
    }
}

void BackendClient::disconnect()
{
    try {
        grav_socket::closeSocket(_socket);
        _socket = grav_socket::invalidHandle();
        _recvBuffer.clear();
        if (_networkInitialized) {
            grav_socket::shutdownSocketLayer();
            _networkInitialized = false;
        }
    } catch (const std::exception &ex) {
        std::cerr << backendClientError("disconnect", ex.what()) << "\n";
        _socket = grav_socket::invalidHandle();
        _recvBuffer.clear();
        _networkInitialized = false;
    } catch (...) {
        std::cerr << backendClientError("disconnect", "non-standard exception") << "\n";
        _socket = grav_socket::invalidHandle();
        _recvBuffer.clear();
        _networkInitialized = false;
    }
}

BackendClientResponse BackendClient::sendJson(const std::string &jsonLine)
{
    BackendClientResponse response;
    try {
        if (!isConnected()) {
            response.error = backendClientError("sendJson", "not connected");
            return response;
        }

        std::string line = trim(jsonLine);
        if (line.empty()) {
            response.error = backendClientError("sendJson", "empty request");
            return response;
        }
        line.push_back('\n');

        if (!sendAll(_socket, asBytes(line))) {
            response.error = backendClientError("sendJson", "send failed");
            disconnect();
            return response;
        }

        if (!readLine(response.raw)) {
            response.error = backendClientError("sendJson", "recv failed");
            disconnect();
            return response;
        }

        grav_protocol::BackendResponseEnvelope envelope{};
        if (!grav_protocol::BackendJsonCodec::parseResponseEnvelope(response.raw, envelope, response.error)) {
            response.error = backendClientError("sendJson", "invalid response");
            return response;
        }
        response.ok = envelope.ok;
        response.error = envelope.error;
        return response;
    } catch (const std::exception &ex) {
        response.ok = false;
        response.error = backendClientError("sendJson", ex.what());
        disconnect();
        return response;
    } catch (...) {
        response.ok = false;
        response.error = backendClientError("sendJson", "non-standard exception");
        disconnect();
        return response;
    }
}

