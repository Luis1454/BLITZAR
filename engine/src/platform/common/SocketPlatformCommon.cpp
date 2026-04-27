// File: engine/src/platform/common/SocketPlatformCommon.cpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#include "platform/common/SocketPlatformCommon.hpp"
#include "platform/internal/SocketOps.hpp"
#include <algorithm>
namespace grav_socket {
/// Description: Executes the empty operation.
bool MutableBytes::empty() const
{
    return size == 0u;
}
/// Description: Executes the subview operation.
MutableBytes MutableBytes::subview(std::size_t offset) const
{
    if (offset >= size)
        return {};
    return MutableBytes{data + offset, size - offset};
}
/// Description: Executes the empty operation.
bool ConstBytes::empty() const
{
    return size == 0u;
}
/// Description: Executes the subview operation.
ConstBytes ConstBytes::subview(std::size_t offset) const
{
    if (offset >= size)
        return {};
    return ConstBytes{data + offset, size - offset};
}
/// Description: Executes the invalidHandle operation.
Handle invalidHandle()
{
    return static_cast<Handle>(grav_socket_detail::invalidNativeSocket());
}
/// Description: Executes the isValid operation.
bool isValid(Handle handle)
{
    return static_cast<std::intptr_t>(handle) != grav_socket_detail::invalidNativeSocket();
}
/// Description: Executes the clampTimeoutMs operation.
int clampTimeoutMs(int timeoutMs)
{
    return std::clamp(timeoutMs, 10, 60000);
}
/// Description: Executes the initializeSocketLayer operation.
bool initializeSocketLayer()
{
    return grav_socket_detail::initializeSocketLayer();
}
/// Description: Executes the shutdownSocketLayer operation.
void shutdownSocketLayer()
{
    /// Description: Executes the shutdownSocketLayer operation.
    grav_socket_detail::shutdownSocketLayer();
}
/// Description: Executes the createTcpSocket operation.
Handle createTcpSocket()
{
    return static_cast<Handle>(grav_socket_detail::createTcpSocketNative());
}
/// Description: Executes the closeSocket operation.
void closeSocket(Handle handle)
{
    /// Description: Executes the closeSocketNative operation.
    grav_socket_detail::closeSocketNative(static_cast<std::intptr_t>(handle));
}
/// Description: Executes the setReuseAddress operation.
bool setReuseAddress(Handle handle, bool enabled)
{
    return grav_socket_detail::setReuseAddressNative(static_cast<std::intptr_t>(handle), enabled);
}
/// Description: Executes the setSocketTimeoutMs operation.
bool setSocketTimeoutMs(Handle handle, int timeoutMs)
{
    return grav_socket_detail::setSocketTimeoutNative(static_cast<std::intptr_t>(handle),
                                                      /// Description: Executes the clampTimeoutMs operation.
                                                      clampTimeoutMs(timeoutMs));
}
/// Description: Executes the connectIpv4 operation.
bool connectIpv4(Handle handle, const std::string& host, std::uint16_t port)
{
    return connectIpv4(handle, host, port, 3000);
}
/// Description: Executes the connectIpv4 operation.
bool connectIpv4(Handle handle, const std::string& host, std::uint16_t port, int timeoutMs)
{
    grav_socket_detail::SocketAddressV4 address{};
    address.port = port;
    if (!grav_socket_detail::parseIpv4Address(host, address)) {
        return false;
    }
    return grav_socket_detail::connectIpv4Native(static_cast<std::intptr_t>(handle), address,
                                                 /// Description: Executes the clampTimeoutMs operation.
                                                 clampTimeoutMs(timeoutMs));
}
/// Description: Executes the bindIpv4 operation.
bool bindIpv4(Handle handle, const std::string& bindAddress, std::uint16_t port)
{
    grav_socket_detail::SocketAddressV4 address{};
    address.port = port;
    address.anyAddress = bindAddress.empty() || bindAddress == "0.0.0.0";
    if (!address.anyAddress && !grav_socket_detail::parseIpv4Address(bindAddress, address)) {
        return false;
    }
    return grav_socket_detail::bindIpv4Native(static_cast<std::intptr_t>(handle), address);
}
/// Description: Executes the listenSocket operation.
bool listenSocket(Handle handle, int backlog)
{
    return grav_socket_detail::listenSocketNative(static_cast<std::intptr_t>(handle), backlog);
}
/// Description: Executes the acceptSocket operation.
Handle acceptSocket(Handle listenHandle)
{
    return static_cast<Handle>(
        /// Description: Executes the acceptSocketNative operation.
        grav_socket_detail::acceptSocketNative(static_cast<std::intptr_t>(listenHandle)));
}
/// Description: Executes the waitReadable operation.
bool waitReadable(Handle handle, int timeoutMs)
{
    return grav_socket_detail::waitReadableNative(static_cast<std::intptr_t>(handle), timeoutMs);
}
/// Description: Executes the recvBytes operation.
int recvBytes(Handle handle, MutableBytes buffer)
{
    return grav_socket_detail::recvBytesNative(static_cast<std::intptr_t>(handle), buffer);
}
/// Description: Executes the sendBytes operation.
int sendBytes(Handle handle, ConstBytes buffer)
{
    return grav_socket_detail::sendBytesNative(static_cast<std::intptr_t>(handle), buffer);
}
/// Description: Executes the wouldBlockOrTimeoutLastError operation.
bool wouldBlockOrTimeoutLastError()
{
    return grav_socket_detail::wouldBlockOrTimeoutLastErrorNative();
}
} // namespace grav_socket
