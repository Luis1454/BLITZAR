// File: engine/src/platform/win/SocketPlatformWin.cpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#include "platform/win/SocketPlatformWin.hpp"
#include <cstring>
#include <winsock2.h>
#include <ws2tcpip.h>

namespace grav_socket_detail {
typedef SOCKET NativeSocket;
static constexpr NativeSocket kInvalidNativeSocket = INVALID_SOCKET;

/// Description: Executes the toNative operation.
static NativeSocket toNative(std::intptr_t handle)
{
    return static_cast<NativeSocket>(handle);
}

/// Description: Executes the toStored operation.
static std::intptr_t toStored(NativeSocket handle)
{
    return static_cast<std::intptr_t>(handle);
}

/// Description: Executes the setNonBlocking operation.
static bool setNonBlocking(NativeSocket socket, bool enabled)
{
    u_long mode = enabled ? 1u : 0u;
    return ::ioctlsocket(socket, FIONBIO, &mode) == 0;
}

/// Description: Executes the toSockaddr operation.
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

/// Description: Executes the invalidNativeSocket operation.
std::intptr_t invalidNativeSocket()
{
    return toStored(kInvalidNativeSocket);
}

/// Description: Executes the initializeSocketLayer operation.
bool initializeSocketLayer()
{
    WSADATA wsaData{};
    return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
}

/// Description: Executes the shutdownSocketLayer operation.
void shutdownSocketLayer()
{
    WSACleanup();
}

/// Description: Executes the createTcpSocketNative operation.
std::intptr_t createTcpSocketNative()
{
    return toStored(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
}

/// Description: Executes the closeSocketNative operation.
void closeSocketNative(std::intptr_t handle)
{
    const NativeSocket socket = toNative(handle);
    if (socket != kInvalidNativeSocket)
        closesocket(socket);
}

/// Description: Executes the setReuseAddressNative operation.
bool setReuseAddressNative(std::intptr_t handle, bool enabled)
{
    const int value = enabled ? 1 : 0;
    return ::setsockopt(toNative(handle), SOL_SOCKET, SO_REUSEADDR,
                        reinterpret_cast<const char*>(&value), sizeof(value)) == 0;
}

/// Description: Executes the setSocketTimeoutNative operation.
bool setSocketTimeoutNative(std::intptr_t handle, int timeoutMs)
{
    const DWORD timeoutValue = static_cast<DWORD>(timeoutMs);
    const bool recvOk =
        ::setsockopt(toNative(handle), SOL_SOCKET, SO_RCVTIMEO,
                     reinterpret_cast<const char*>(&timeoutValue), sizeof(timeoutValue)) == 0;
    const bool sendOk =
        ::setsockopt(toNative(handle), SOL_SOCKET, SO_SNDTIMEO,
                     reinterpret_cast<const char*>(&timeoutValue), sizeof(timeoutValue)) == 0;
    return recvOk && sendOk;
}

/// Description: Executes the parseIpv4Address operation.
bool parseIpv4Address(const std::string& host, SocketAddressV4& outAddress)
{
    in_addr addr{};
    if (::inet_pton(AF_INET, host.c_str(), &addr) != 1)
        return false;
    std::memcpy(outAddress.addressBytes.data(), &addr.s_addr, outAddress.addressBytes.size());
    outAddress.anyAddress = false;
    return true;
}

/// Description: Executes the connectIpv4Native operation.
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
    const int connectError = WSAGetLastError();
    if (connectError != WSAEWOULDBLOCK && connectError != WSAEINPROGRESS &&
        connectError != WSAEALREADY) {
        (void)setNonBlocking(socket, false);
        return false;
    }
    fd_set writeSet;
    fd_set errorSet;
    FD_ZERO(&writeSet);
    FD_ZERO(&errorSet);
    FD_SET(socket, &writeSet);
    FD_SET(socket, &errorSet);
    timeval timeout{};
    timeout.tv_sec = timeoutMs / 1000;
    timeout.tv_usec = (timeoutMs % 1000) * 1000;
    if (::select(0, nullptr, &writeSet, &errorSet, &timeout) <= 0) {
        (void)setNonBlocking(socket, false);
        return false;
    }
    int soError = 0;
    int soErrorLen = static_cast<int>(sizeof(soError));
    const int getOptResult =
        ::getsockopt(socket, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&soError), &soErrorLen);
    (void)setNonBlocking(socket, false);
    return getOptResult == 0 && soError == 0;
}

/// Description: Executes the bindIpv4Native operation.
bool bindIpv4Native(std::intptr_t handle, const SocketAddressV4& address)
{
    sockaddr_in endpoint = toSockaddr(address);
    return ::bind(toNative(handle), reinterpret_cast<sockaddr*>(&endpoint), sizeof(endpoint)) == 0;
}

/// Description: Executes the listenSocketNative operation.
bool listenSocketNative(std::intptr_t handle, int backlog)
{
    return ::listen(toNative(handle), backlog) == 0;
}

/// Description: Executes the acceptSocketNative operation.
std::intptr_t acceptSocketNative(std::intptr_t handle)
{
    sockaddr_in clientAddr{};
    int clientAddrLen = static_cast<int>(sizeof(clientAddr));
    return toStored(
        ::accept(toNative(handle), reinterpret_cast<sockaddr*>(&clientAddr), &clientAddrLen));
}

/// Description: Executes the waitReadableNative operation.
bool waitReadableNative(std::intptr_t handle, int timeoutMs)
{
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(toNative(handle), &readSet);
    timeval timeout{};
    timeout.tv_sec = timeoutMs / 1000;
    timeout.tv_usec = (timeoutMs % 1000) * 1000;
    return ::select(0, &readSet, nullptr, nullptr, &timeout) > 0;
}

/// Description: Executes the recvBytesNative operation.
int recvBytesNative(std::intptr_t handle, grav_socket::MutableBytes buffer)
{
    return buffer.empty() ? 0
                          : ::recv(toNative(handle), reinterpret_cast<char*>(buffer.data),
                                   static_cast<int>(buffer.size), 0);
}

/// Description: Executes the sendBytesNative operation.
int sendBytesNative(std::intptr_t handle, grav_socket::ConstBytes buffer)
{
    return buffer.empty() ? 0
                          : ::send(toNative(handle), reinterpret_cast<const char*>(buffer.data),
                                   static_cast<int>(buffer.size), 0);
}

/// Description: Executes the wouldBlockOrTimeoutLastErrorNative operation.
bool wouldBlockOrTimeoutLastErrorNative()
{
    const int lastError = WSAGetLastError();
    return lastError == WSAETIMEDOUT || lastError == WSAEWOULDBLOCK;
}
} // namespace grav_socket_detail
