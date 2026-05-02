/*
 * @file runtime/src/server/daemon/helpers.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Helper utilities for network transport and error formatting.
 */

#include "server/ServerDaemon.hpp"
#include "server/daemon/helpers.hpp"
#include "platform/SocketPlatform.hpp"
#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>

/*
 * @brief Trim leading and trailing whitespace from a string.
 * @param input Input value used by this contract.
 * @return std::string value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
std::string daemonTrim(const std::string& input)
{
    const auto begin = std::find_if_not(input.begin(), input.end(), [](unsigned char c) {
        return std::isspace(c) != 0;
    });
    const auto end = std::find_if_not(input.rbegin(), input.rend(), [](unsigned char c) {
                         return std::isspace(c) != 0;
                     }).base();
    if (begin >= end)
        return {};
    return std::string(begin, end);
}

/*
 * @brief Convert string_view to binary bytes.
 * @param text Input value used by this contract.
 * @return bltzr_socket::ConstBytes value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bltzr_socket::ConstBytes daemonAsBytes(std::string_view text)
{
    return bltzr_socket::ConstBytes{reinterpret_cast<const std::byte*>(text.data()), text.size()};
}

/*
 * @brief Send all bytes on a socket with automatic retry.
 * @param socketHandle Input value used by this contract.
 * @param bytes Input value used by this contract.
 * @return bool value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool daemonSendAll(bltzr_socket::Handle socketHandle, bltzr_socket::ConstBytes bytes)
{
    std::size_t offset = 0;
    while (offset < bytes.size) {
        const int sent = bltzr_socket::sendBytes(socketHandle, bytes.subview(offset));
        if (sent <= 0)
            return false;
        offset += static_cast<std::size_t>(sent);
    }
    return true;
}

/*
 * @brief Format standardized error message for daemon operations.
 * @param operation Input value used by this contract.
 * @param detail Input value used by this contract.
 * @return std::string value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
std::string daemonServerError(std::string_view operation, std::string_view detail)
{
    return std::string("[ipc] ") + std::string(operation) + ": " + std::string(detail);
}
