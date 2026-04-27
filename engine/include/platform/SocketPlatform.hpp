// File: engine/include/platform/SocketPlatform.hpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#ifndef GRAVITY_ENGINE_INCLUDE_PLATFORM_SOCKETPLATFORM_HPP_
#define GRAVITY_ENGINE_INCLUDE_PLATFORM_SOCKETPLATFORM_HPP_
#include <cstddef>
#include <cstdint>
#include <string>

namespace grav_socket {
typedef std::intptr_t Handle;

/// Description: Defines the MutableBytes data or behavior contract.
struct MutableBytes {
    std::byte* data = nullptr;
    std::size_t size = 0u;
    /// Description: Describes the empty operation contract.
    bool empty() const;
    /// Description: Describes the subview operation contract.
    MutableBytes subview(std::size_t offset) const;
};

/// Description: Defines the ConstBytes data or behavior contract.
struct ConstBytes {
    const std::byte* data = nullptr;
    std::size_t size = 0u;
    /// Description: Describes the empty operation contract.
    bool empty() const;
    /// Description: Describes the subview operation contract.
    ConstBytes subview(std::size_t offset) const;
};

/// Description: Executes the invalidHandle operation.
Handle invalidHandle();
/// Description: Executes the isValid operation.
bool isValid(Handle handle);
/// Description: Executes the clampTimeoutMs operation.
int clampTimeoutMs(int timeoutMs);
/// Description: Executes the initializeSocketLayer operation.
bool initializeSocketLayer();
/// Description: Executes the shutdownSocketLayer operation.
void shutdownSocketLayer();
/// Description: Executes the createTcpSocket operation.
Handle createTcpSocket();
/// Description: Executes the closeSocket operation.
void closeSocket(Handle handle);
/// Description: Executes the setReuseAddress operation.
bool setReuseAddress(Handle handle, bool enabled);
/// Description: Executes the setSocketTimeoutMs operation.
bool setSocketTimeoutMs(Handle handle, int timeoutMs);
/// Description: Executes the connectIpv4 operation.
bool connectIpv4(Handle handle, const std::string& host, std::uint16_t port);
/// Description: Executes the connectIpv4 operation.
bool connectIpv4(Handle handle, const std::string& host, std::uint16_t port, int timeoutMs);
/// Description: Executes the bindIpv4 operation.
bool bindIpv4(Handle handle, const std::string& bindAddress, std::uint16_t port);
/// Description: Executes the listenSocket operation.
bool listenSocket(Handle handle, int backlog);
/// Description: Executes the acceptSocket operation.
Handle acceptSocket(Handle listenHandle);
/// Description: Executes the waitReadable operation.
bool waitReadable(Handle handle, int timeoutMs);
/// Description: Executes the recvBytes operation.
int recvBytes(Handle handle, MutableBytes buffer);
/// Description: Executes the sendBytes operation.
int sendBytes(Handle handle, ConstBytes buffer);
/// Description: Executes the wouldBlockOrTimeoutLastError operation.
bool wouldBlockOrTimeoutLastError();
} // namespace grav_socket
#endif // GRAVITY_ENGINE_INCLUDE_PLATFORM_SOCKETPLATFORM_HPP_
