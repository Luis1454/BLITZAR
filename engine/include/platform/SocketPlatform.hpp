/*
 * @file engine/include/platform/SocketPlatform.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Platform abstraction interfaces for portable runtime services.
 */

#ifndef BLITZAR_ENGINE_INCLUDE_PLATFORM_SOCKETPLATFORM_HPP_
#define BLITZAR_ENGINE_INCLUDE_PLATFORM_SOCKETPLATFORM_HPP_
#include <cstddef>
#include <cstdint>
#include <string>

namespace bltzr_socket {
typedef std::intptr_t Handle;

struct MutableBytes {
    std::byte* data = nullptr;
    std::size_t size = 0u;
    bool empty() const;
    MutableBytes subview(std::size_t offset) const;
};

struct ConstBytes {
    const std::byte* data = nullptr;
    std::size_t size = 0u;
    bool empty() const;
    ConstBytes subview(std::size_t offset) const;
};

Handle invalidHandle();
bool isValid(Handle handle);
int clampTimeoutMs(int timeoutMs);
bool initializeSocketLayer();
void shutdownSocketLayer();
Handle createTcpSocket();
void closeSocket(Handle handle);
bool setReuseAddress(Handle handle, bool enabled);
bool setSocketTimeoutMs(Handle handle, int timeoutMs);
bool connectIpv4(Handle handle, const std::string& host, std::uint16_t port);
bool connectIpv4(Handle handle, const std::string& host, std::uint16_t port, int timeoutMs);
bool bindIpv4(Handle handle, const std::string& bindAddress, std::uint16_t port);
bool listenSocket(Handle handle, int backlog);
Handle acceptSocket(Handle listenHandle);
bool waitReadable(Handle handle, int timeoutMs);
int recvBytes(Handle handle, MutableBytes buffer);
int sendBytes(Handle handle, ConstBytes buffer);
bool wouldBlockOrTimeoutLastError();
} // namespace bltzr_socket
#endif // BLITZAR_ENGINE_INCLUDE_PLATFORM_SOCKETPLATFORM_HPP_
