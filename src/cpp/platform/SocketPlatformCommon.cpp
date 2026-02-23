#include "sim/SocketPlatform.hpp"

#include "src/cpp/platform/internal/SocketOps.hpp"

#include <algorithm>

namespace sim::socket {

Handle invalidHandle()
{
    return static_cast<Handle>(detail::invalidNativeSocket());
}

bool isValid(Handle handle)
{
    return static_cast<std::intptr_t>(handle) != detail::invalidNativeSocket();
}

int clampTimeoutMs(int timeoutMs)
{
    return std::clamp(timeoutMs, 10, 60000);
}

bool initializeSocketLayer()
{
    return detail::initializeSocketLayer();
}

void shutdownSocketLayer()
{
    detail::shutdownSocketLayer();
}

Handle createTcpSocket()
{
    return static_cast<Handle>(detail::createTcpSocketNative());
}

void closeSocket(Handle handle)
{
    detail::closeSocketNative(static_cast<std::intptr_t>(handle));
}

bool setReuseAddress(Handle handle, bool enabled)
{
    return detail::setReuseAddressNative(static_cast<std::intptr_t>(handle), enabled);
}

bool setSocketTimeoutMs(Handle handle, int timeoutMs)
{
    return detail::setSocketTimeoutNative(static_cast<std::intptr_t>(handle), clampTimeoutMs(timeoutMs));
}

bool connectIpv4(Handle handle, const std::string &host, std::uint16_t port)
{
    return connectIpv4(handle, host, port, 3000);
}

bool connectIpv4(Handle handle, const std::string &host, std::uint16_t port, int timeoutMs)
{
    detail::SocketAddressV4 address{};
    address.port = port;
    if (!detail::parseIpv4Address(host, address)) {
        return false;
    }
    return detail::connectIpv4Native(
        static_cast<std::intptr_t>(handle),
        address,
        clampTimeoutMs(timeoutMs));
}

bool bindIpv4(Handle handle, const std::string &bindAddress, std::uint16_t port)
{
    detail::SocketAddressV4 address{};
    address.port = port;
    address.anyAddress = bindAddress.empty() || bindAddress == "0.0.0.0";
    if (!address.anyAddress && !detail::parseIpv4Address(bindAddress, address)) {
        return false;
    }
    return detail::bindIpv4Native(static_cast<std::intptr_t>(handle), address);
}

bool listenSocket(Handle handle, int backlog)
{
    return detail::listenSocketNative(static_cast<std::intptr_t>(handle), backlog);
}

Handle acceptSocket(Handle listenHandle)
{
    return static_cast<Handle>(detail::acceptSocketNative(static_cast<std::intptr_t>(listenHandle)));
}

bool waitReadable(Handle handle, int timeoutMs)
{
    return detail::waitReadableNative(static_cast<std::intptr_t>(handle), timeoutMs);
}

int recvBytes(Handle handle, MutableBytes buffer)
{
    return detail::recvBytesNative(static_cast<std::intptr_t>(handle), buffer);
}

int sendBytes(Handle handle, ConstBytes buffer)
{
    return detail::sendBytesNative(static_cast<std::intptr_t>(handle), buffer);
}

bool wouldBlockOrTimeoutLastError()
{
    return detail::wouldBlockOrTimeoutLastErrorNative();
}

} // namespace sim::socket
