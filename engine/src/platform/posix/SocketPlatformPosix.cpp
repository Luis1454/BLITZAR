// File: engine/src/platform/posix/SocketPlatformPosix.cpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#include "platform/posix/SocketPlatformPosix.hpp"
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
namespace grav_socket_detail {
typedef int NativeSocket;
static constexpr NativeSocket kInvalidNativeSocket = -1;
static NativeSocket toNative(std::intptr_t handle)
{
    return static_cast<NativeSocket>(handle);
}
static std::intptr_t toStored(NativeSocket handle)
{
    return static_cast<std::intptr_t>(handle);
}
static bool setNonBlocking(NativeSocket socket, bool enabled)
{
    const int flags = ::fcntl(socket, F_GETFL, 0);
    if (flags < 0)
        return false;
    return ::fcntl(socket, F_SETFL, enabled ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK)) == 0;
}
static sockaddr_in toSockaddr(const SocketAddressV4& address)
{
    sockaddr_in out{};
    out.sin_family = AF_INET;
    out.sin_port = htons(address.port);
    if (address.anyAddress) {
        out.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    else {
        std::memcpy(&out.sin_addr.s_addr, address.addressBytes.data(), address.addressBytes.size());
    }
    return out;
}
std::intptr_t invalidNativeSocket()
{
    return toStored(kInvalidNativeSocket);
}
bool initializeSocketLayer()
{
    return true;
}
void shutdownSocketLayer()
{
}
std::intptr_t createTcpSocketNative()
{
    return toStored(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
}
void closeSocketNative(std::intptr_t handle)
{
    const NativeSocket socket = toNative(handle);
    if (socket != kInvalidNativeSocket)
        close(socket);
}
bool setReuseAddressNative(std::intptr_t handle, bool enabled)
{
    const int value = enabled ? 1 : 0;
    return ::setsockopt(toNative(handle), SOL_SOCKET, SO_REUSEADDR,
                        reinterpret_cast<const char*>(&value), sizeof(value)) == 0;
}
bool setSocketTimeoutNative(std::intptr_t handle, int timeoutMs)
{
    timeval timeout{};
    timeout.tv_sec = timeoutMs / 1000;
    timeout.tv_usec = (timeoutMs % 1000) * 1000;
    const bool recvOk =
        ::setsockopt(toNative(handle), SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == 0;
    const bool sendOk =
        ::setsockopt(toNative(handle), SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) == 0;
    return recvOk && sendOk;
}
bool parseIpv4Address(const std::string& host, SocketAddressV4& outAddress)
{
    in_addr addr{};
    if (::inet_pton(AF_INET, host.c_str(), &addr) != 1)
        return false;
    std::memcpy(outAddress.addressBytes.data(), &addr.s_addr, outAddress.addressBytes.size());
    outAddress.anyAddress = false;
    return true;
}
bool connectIpv4Native(std::intptr_t handle, const SocketAddressV4& address, int timeoutMs)
{
    const NativeSocket socket = toNative(handle);
    sockaddr_in endpoint = toSockaddr(address);
    if (!setNonBlocking(socket, true))
        return false;
    if (::connect(socket, reinterpret_cast<const sockaddr*>(&endpoint), sizeof(endpoint)) == 0) {
        (void)setNonBlocking(socket, false);
        return true;
    }
    if (errno != EINPROGRESS && errno != EWOULDBLOCK && errno != EALREADY) {
        (void)setNonBlocking(socket, false);
        return false;
    }
    fd_set writeSet;
    FD_ZERO(&writeSet);
    FD_SET(socket, &writeSet);
    timeval timeout{};
    timeout.tv_sec = timeoutMs / 1000;
    timeout.tv_usec = (timeoutMs % 1000) * 1000;
    if (::select(socket + 1, nullptr, &writeSet, nullptr, &timeout) <= 0) {
        (void)setNonBlocking(socket, false);
        return false;
    }
    int soError = 0;
    socklen_t soErrorLen = static_cast<socklen_t>(sizeof(soError));
    const int getOptResult =
        ::getsockopt(socket, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&soError), &soErrorLen);
    (void)setNonBlocking(socket, false);
    return getOptResult == 0 && soError == 0;
}
bool bindIpv4Native(std::intptr_t handle, const SocketAddressV4& address)
{
    sockaddr_in endpoint = toSockaddr(address);
    return ::bind(toNative(handle), reinterpret_cast<sockaddr*>(&endpoint), sizeof(endpoint)) == 0;
}
bool listenSocketNative(std::intptr_t handle, int backlog)
{
    return ::listen(toNative(handle), backlog) == 0;
}
std::intptr_t acceptSocketNative(std::intptr_t handle)
{
    sockaddr_in clientAddr{};
    socklen_t clientAddrLen = static_cast<socklen_t>(sizeof(clientAddr));
    return toStored(
        ::accept(toNative(handle), reinterpret_cast<sockaddr*>(&clientAddr), &clientAddrLen));
}
bool waitReadableNative(std::intptr_t handle, int timeoutMs)
{
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(toNative(handle), &readSet);
    timeval timeout{};
    timeout.tv_sec = timeoutMs / 1000;
    timeout.tv_usec = (timeoutMs % 1000) * 1000;
    return ::select(static_cast<int>(toNative(handle) + 1), &readSet, nullptr, nullptr, &timeout) >
           0;
}
int recvBytesNative(std::intptr_t handle, grav_socket::MutableBytes buffer)
{
    return buffer.empty() ? 0
                          : static_cast<int>(::recv(toNative(handle), buffer.data, buffer.size, 0));
}
int sendBytesNative(std::intptr_t handle, grav_socket::ConstBytes buffer)
{
    return buffer.empty() ? 0
                          : static_cast<int>(::send(toNative(handle), buffer.data, buffer.size, 0));
}
bool wouldBlockOrTimeoutLastErrorNative()
{
    return errno == EAGAIN || errno == EWOULDBLOCK;
}
} // namespace grav_socket_detail
