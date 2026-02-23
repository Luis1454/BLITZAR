#include "src/cpp/platform/internal/SocketOps.hpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>

#include <cstring>

namespace sim::socket::detail {
using NativeSocket = SOCKET;
static constexpr NativeSocket kInvalidNativeSocket = INVALID_SOCKET;

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
    u_long mode = enabled ? 1u : 0u;
    return ::ioctlsocket(socket, FIONBIO, &mode) == 0;
}

static sockaddr_in toSockaddr(const SocketAddressV4 &address)
{
    sockaddr_in out{};
    out.sin_family = AF_INET;
    out.sin_port = htons(address.port);
    if (address.anyAddress) {
        out.sin_addr.s_addr = htonl(INADDR_ANY);
    } else {
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
    WSADATA wsaData{};
    return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
}

void shutdownSocketLayer()
{
    WSACleanup();
}

std::intptr_t createTcpSocketNative()
{
    return toStored(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
}

void closeSocketNative(std::intptr_t handle)
{
    const NativeSocket socket = toNative(handle);
    if (socket == kInvalidNativeSocket) {
        return;
    }
    closesocket(socket);
}

bool setReuseAddressNative(std::intptr_t handle, bool enabled)
{
    const int value = enabled ? 1 : 0;
    return setsockopt(
               toNative(handle),
               SOL_SOCKET,
               SO_REUSEADDR,
               reinterpret_cast<const char *>(&value),
               sizeof(value))
        == 0;
}

bool setSocketTimeoutNative(std::intptr_t handle, int timeoutMs)
{
    const DWORD timeoutValue = static_cast<DWORD>(timeoutMs);
    const bool recvOk = ::setsockopt(
                            toNative(handle),
                            SOL_SOCKET,
                            SO_RCVTIMEO,
                            reinterpret_cast<const char *>(&timeoutValue),
                            sizeof(timeoutValue))
        == 0;
    const bool sendOk = ::setsockopt(
                            toNative(handle),
                            SOL_SOCKET,
                            SO_SNDTIMEO,
                            reinterpret_cast<const char *>(&timeoutValue),
                            sizeof(timeoutValue))
        == 0;
    return recvOk && sendOk;
}

bool parseIpv4Address(const std::string &host, SocketAddressV4 &outAddress)
{
    in_addr addr{};
    if (::inet_pton(AF_INET, host.c_str(), &addr) != 1) {
        return false;
    }
    std::memcpy(outAddress.addressBytes.data(), &addr.s_addr, outAddress.addressBytes.size());
    outAddress.anyAddress = false;
    return true;
}

bool connectIpv4Native(std::intptr_t handle, const SocketAddressV4 &address, int timeoutMs)
{
    const NativeSocket socket = toNative(handle);
    sockaddr_in endpoint = toSockaddr(address);
    if (!setNonBlocking(socket, true)) {
        return false;
    }

    const int connectResult = ::connect(
        socket,
        reinterpret_cast<const sockaddr *>(&endpoint),
        sizeof(endpoint));
    if (connectResult == 0) {
        (void)setNonBlocking(socket, false);
        return true;
    }

    const int connectError = WSAGetLastError();
    if (connectError != WSAEWOULDBLOCK && connectError != WSAEINPROGRESS && connectError != WSAEALREADY) {
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

    const int ready = ::select(0, nullptr, &writeSet, &errorSet, &timeout);
    if (ready <= 0) {
        (void)setNonBlocking(socket, false);
        return false;
    }

    int soError = 0;
    int soErrorLen = static_cast<int>(sizeof(soError));
    const int getOptResult = ::getsockopt(
        socket,
        SOL_SOCKET,
        SO_ERROR,
        reinterpret_cast<char *>(&soError),
        &soErrorLen);

    (void)setNonBlocking(socket, false);
    return getOptResult == 0 && soError == 0;
}

bool bindIpv4Native(std::intptr_t handle, const SocketAddressV4 &address)
{
    sockaddr_in endpoint = toSockaddr(address);
    return ::bind(
               toNative(handle),
               reinterpret_cast<sockaddr *>(&endpoint),
               sizeof(endpoint))
        == 0;
}

bool listenSocketNative(std::intptr_t handle, int backlog)
{
    return ::listen(toNative(handle), backlog) == 0;
}

std::intptr_t acceptSocketNative(std::intptr_t handle)
{
    sockaddr_in clientAddr{};
    int clientAddrLen = static_cast<int>(sizeof(clientAddr));
    const NativeSocket clientSocket = ::accept(
        toNative(handle),
        reinterpret_cast<sockaddr *>(&clientAddr),
        &clientAddrLen);
    return toStored(clientSocket);
}

bool waitReadableNative(std::intptr_t handle, int timeoutMs)
{
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(toNative(handle), &readSet);

    timeval timeout{};
    timeout.tv_sec = timeoutMs / 1000;
    timeout.tv_usec = (timeoutMs % 1000) * 1000;

    const int ready = ::select(0, &readSet, nullptr, nullptr, &timeout);
    return ready > 0;
}

int recvBytesNative(std::intptr_t handle, MutableBytes buffer)
{
    if (buffer.empty()) {
        return 0;
    }
    return ::recv(
        toNative(handle),
        reinterpret_cast<char *>(buffer.data),
        static_cast<int>(buffer.size),
        0);
}

int sendBytesNative(std::intptr_t handle, ConstBytes buffer)
{
    if (buffer.empty()) {
        return 0;
    }
    return ::send(
        toNative(handle),
        reinterpret_cast<const char *>(buffer.data),
        static_cast<int>(buffer.size),
        0);
}

bool wouldBlockOrTimeoutLastErrorNative()
{
    const int lastError = WSAGetLastError();
    return lastError == WSAETIMEDOUT || lastError == WSAEWOULDBLOCK;
}

} // namespace sim::socket::detail
