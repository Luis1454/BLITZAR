#include "protocol/BackendClientIo.hpp"
#include "platform/SocketPlatform.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <exception>
#include <iostream>
#include <string>
#include <string_view>

static std::string backendClientError(std::string_view operation, std::string_view detail)
{
    return std::string("[backend-client] ") + std::string(operation) + ": " + std::string(detail);
}

std::string BackendClient::trim(const std::string &value)
{
    const auto begin = std::find_if_not(value.begin(), value.end(), [](unsigned char c) {
        return std::isspace(c) != 0;
    });
    const auto end = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char c) {
        return std::isspace(c) != 0;
    }).base();
    if (begin >= end) {
        return {};
    }
    return std::string(begin, end);
}

bool BackendClient::readLine(std::string &outLine)
{
    try {
        if (!grav_socket::isValid(_socket)) {
            return false;
        }

        std::size_t pos = _recvBuffer.find('\n');
        while (pos == std::string::npos) {
            std::array<char, 2048> chunk{};
            const int received = grav_socket::recvBytes(
                _socket,
                grav_socket::MutableBytes{
                    reinterpret_cast<std::byte *>(chunk.data()),
                    chunk.size()});
            if (received <= 0) {
                return false;
            }
            _recvBuffer.append(chunk.data(), static_cast<std::size_t>(received));
            if (_recvBuffer.size() > (512u * 1024u)) {
                return false;
            }
            pos = _recvBuffer.find('\n');
        }

        outLine = _recvBuffer.substr(0, pos);
        _recvBuffer.erase(0, pos + 1);
        if (!outLine.empty() && outLine.back() == '\r') {
            outLine.pop_back();
        }
        return true;
    } catch (const std::exception &ex) {
        std::cerr << backendClientError("readLine", ex.what()) << "\n";
        return false;
    } catch (...) {
        std::cerr << backendClientError("readLine", "non-standard exception") << "\n";
        return false;
    }
}
