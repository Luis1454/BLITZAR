#include "platform/common/SocketPlatformCommon.hpp"

#include "platform/internal/SocketOps.hpp"

#include <algorithm>

namespace grav_socket {

bool MutableBytes::empty() const
{
    return size == 0u;
}

MutableBytes MutableBytes::subview(std::size_t offset) const
{
    if (offset >= size) {
        return {};
    }
    return MutableBytes{data + offset, size - offset};
}

bool ConstBytes::empty() const
{
    return size == 0u;
}

ConstBytes ConstBytes::subview(std::size_t offset) const
{
    if (offset >= size) {
        return {};
    }
    return ConstBytes{data + offset, size - offset};
}

Handle invalidHandle()
{
    return static_cast<Handle>(grav_socket_detail::invalidNativeSocket());
}

bool isValid(Handle handle)
{
    return static_cast<std::intptr_t>(handle) != grav_socket_detail::invalidNativeSocket();
}

int clampTimeoutMs(int timeoutMs)
{
    return std::clamp(timeoutMs, 10, 60000);
}

bool initializeSocketLayer()
{
    return grav_socket_detail::initializeSocketLayer();
}

void shutdownSocketLayer()
{
    grav_socket_detail::shutdownSocketLayer();
}

Handle createTcpSocket()
{
    return static_cast<Handle>(grav_socket_detail::createTcpSocketNative());
}

void closeSocket(Handle handle)
{
    grav_socket_detail::closeSocketNative(static_cast<std::intptr_t>(handle));
}

bool setReuseAddress(Handle handle, bool enabled)
{
    return grav_socket_detail::setReuseAddressNative(static_cast<std::intptr_t>(handle), enabled);
}

bool setSocketTimeoutMs(Handle handle, int timeoutMs)
{
    return grav_socket_detail::setSocketTimeoutNative(static_cast<std::intptr_t>(handle), clampTimeoutMs(timeoutMs));
}

bool connectIpv4(Handle handle, const std::string &host, std::uint16_t port)
{
    return connectIpv4(handle, host, port, 3000);
}

bool connectIpv4(Handle handle, const std::string &host, std::uint16_t port, int timeoutMs)
{
    grav_socket_detail::SocketAddressV4 address{};
    address.port = port;
    if (!grav_socket_detail::parseIpv4Address(host, address)) {
        return false;
    }
    return grav_socket_detail::connectIpv4Native(
        static_cast<std::intptr_t>(handle),
        address,
        clampTimeoutMs(timeoutMs));
}

bool bindIpv4(Handle handle, const std::string &bindAddress, std::uint16_t port)
{
    grav_socket_detail::SocketAddressV4 address{};
    address.port = port;
    address.anyAddress = bindAddress.empty() || bindAddress == "0.0.0.0";
    if (!address.anyAddress && !grav_socket_detail::parseIpv4Address(bindAddress, address)) {
        return false;
    }
    return grav_socket_detail::bindIpv4Native(static_cast<std::intptr_t>(handle), address);
}

bool listenSocket(Handle handle, int backlog)
{
    return grav_socket_detail::listenSocketNative(static_cast<std::intptr_t>(handle), backlog);
}

Handle acceptSocket(Handle listenHandle)
{
    return static_cast<Handle>(grav_socket_detail::acceptSocketNative(static_cast<std::intptr_t>(listenHandle)));
}

bool waitReadable(Handle handle, int timeoutMs)
{
    return grav_socket_detail::waitReadableNative(static_cast<std::intptr_t>(handle), timeoutMs);
}

int recvBytes(Handle handle, MutableBytes buffer)
{
    return grav_socket_detail::recvBytesNative(static_cast<std::intptr_t>(handle), buffer);
}

int sendBytes(Handle handle, ConstBytes buffer)
{
    return grav_socket_detail::sendBytesNative(static_cast<std::intptr_t>(handle), buffer);
}

bool wouldBlockOrTimeoutLastError()
{
    return grav_socket_detail::wouldBlockOrTimeoutLastErrorNative();
}

} // namespace grav_socket
