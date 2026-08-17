/*
 * @file engine/platform/src/win/Socket.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Platform abstraction implementation for portable runtime services.
 */

#include "internal/SocketOps.hpp"
#include <winsock2.h>
#include <ws2tcpip.h>

namespace bltzr_socket_detail {

sockaddr_in makeSockaddr(const bltzr_socket_detail::SocketAddressV4& address)
{
    sockaddr_in nativeAddress{};
    nativeAddress.sin_family = AF_INET;
    nativeAddress.sin_port = htons(address.port);
    if (address.anyAddress) {
        nativeAddress.sin_addr.s_addr = htonl(INADDR_ANY);
        return nativeAddress;
    }
    const std::uint32_t hostAddress =
        (static_cast<std::uint32_t>(address.addressBytes[0]) << 24u) |
        (static_cast<std::uint32_t>(address.addressBytes[1]) << 16u) |
        (static_cast<std::uint32_t>(address.addressBytes[2]) << 8u) |
        static_cast<std::uint32_t>(address.addressBytes[3]);
    nativeAddress.sin_addr.s_addr = htonl(hostAddress);
    return nativeAddress;
}

std::intptr_t invalidNativeSocket()
{
    return static_cast<std::intptr_t>(INVALID_SOCKET);
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
    const SOCKET socketHandle = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    return static_cast<std::intptr_t>(socketHandle);
}

void closeSocketNative(std::intptr_t handle)
{
    if (handle != invalidNativeSocket()) {
        closesocket(static_cast<SOCKET>(handle));
    }
}

bool setReuseAddressNative(std::intptr_t handle, bool enabled)
{
    const BOOL option = enabled ? TRUE : FALSE;
    return setsockopt(static_cast<SOCKET>(handle), SOL_SOCKET, SO_REUSEADDR,
                      reinterpret_cast<const char*>(&option), sizeof(option)) == 0;
}

bool setSocketTimeoutNative(std::intptr_t handle, int timeoutMs)
{
    if (timeoutMs < 0) {
        return true;
    }
    const DWORD timeout = static_cast<DWORD>(timeoutMs);
    return setsockopt(static_cast<SOCKET>(handle), SOL_SOCKET, SO_RCVTIMEO,
                      reinterpret_cast<const char*>(&timeout), sizeof(timeout)) == 0;
}

bool parseIpv4Address(const std::string& host, SocketAddressV4& outAddress)
{
    if (host.empty() || host == "0.0.0.0") {
        outAddress.addressBytes = {0, 0, 0, 0};
        outAddress.anyAddress = true;
        return true;
    }

    IN_ADDR nativeAddress{};
    if (InetPtonA(AF_INET, host.c_str(), &nativeAddress) != 1) {
        return false;
    }
    const std::uint32_t hostAddress = ntohl(nativeAddress.S_un.S_addr);
    outAddress.addressBytes[0] = static_cast<unsigned char>((hostAddress >> 24u) & 0xFFu);
    outAddress.addressBytes[1] = static_cast<unsigned char>((hostAddress >> 16u) & 0xFFu);
    outAddress.addressBytes[2] = static_cast<unsigned char>((hostAddress >> 8u) & 0xFFu);
    outAddress.addressBytes[3] = static_cast<unsigned char>(hostAddress & 0xFFu);
    outAddress.anyAddress = false;
    return true;
}

bool connectIpv4Native(std::intptr_t handle, const SocketAddressV4& address, int timeoutMs)
{
    const SOCKET socketHandle = static_cast<SOCKET>(handle);
    sockaddr_in nativeAddress = makeSockaddr(address);
    u_long nonBlocking = 1u;
    if (ioctlsocket(socketHandle, FIONBIO, &nonBlocking) != 0) {
        return false;
    }
    bool connected = connect(socketHandle, reinterpret_cast<const sockaddr*>(&nativeAddress),
                             sizeof(nativeAddress)) == 0;
    if (!connected) {
        const int error = WSAGetLastError();
        if (error != WSAEWOULDBLOCK && error != WSAEINPROGRESS && error != WSAEALREADY) {
            connected = false;
        } else {
            fd_set writeSet;
            fd_set errorSet;
            FD_ZERO(&writeSet);
            FD_ZERO(&errorSet);
            FD_SET(socketHandle, &writeSet);
            FD_SET(socketHandle, &errorSet);
            timeval timeout{};
            timeout.tv_sec = timeoutMs / 1000;
            timeout.tv_usec = (timeoutMs % 1000) * 1000;
            const int ready = select(0, nullptr, &writeSet, &errorSet, &timeout);
            if (ready > 0 && FD_ISSET(socketHandle, &writeSet) &&
                !FD_ISSET(socketHandle, &errorSet)) {
                int socketError = 0;
                int socketErrorSize = sizeof(socketError);
                connected = getsockopt(socketHandle, SOL_SOCKET, SO_ERROR,
                                       reinterpret_cast<char*>(&socketError),
                                       &socketErrorSize) == 0 &&
                            socketError == 0;
            }
        }
    }
    u_long blocking = 0u;
    const bool restored = ioctlsocket(socketHandle, FIONBIO, &blocking) == 0;
    return connected && restored;
}

bool bindIpv4Native(std::intptr_t handle, const SocketAddressV4& address)
{
    sockaddr_in nativeAddress = makeSockaddr(address);
    return bind(static_cast<SOCKET>(handle), reinterpret_cast<const sockaddr*>(&nativeAddress),
                sizeof(nativeAddress)) == 0;
}

bool listenSocketNative(std::intptr_t handle, int backlog)
{
    return listen(static_cast<SOCKET>(handle), backlog) == 0;
}

std::intptr_t acceptSocketNative(std::intptr_t handle)
{
    const SOCKET socketHandle = accept(static_cast<SOCKET>(handle), nullptr, nullptr);
    return static_cast<std::intptr_t>(socketHandle);
}

bool waitReadableNative(std::intptr_t handle, int timeoutMs)
{
    fd_set set;
    FD_ZERO(&set);
    FD_SET(static_cast<SOCKET>(handle), &set);
    timeval timeout{};
    timeval* timeoutPtr = nullptr;
    if (timeoutMs >= 0) {
        timeout.tv_sec = timeoutMs / 1000;
        timeout.tv_usec = (timeoutMs % 1000) * 1000;
        timeoutPtr = &timeout;
    }
    return select(0, &set, nullptr, nullptr, timeoutPtr) > 0;
}

int recvBytesNative(std::intptr_t handle, bltzr_socket::MutableBytes buffer)
{
    return recv(static_cast<SOCKET>(handle), reinterpret_cast<char*>(buffer.data),
                static_cast<int>(buffer.size), 0);
}

int sendBytesNative(std::intptr_t handle, bltzr_socket::ConstBytes buffer)
{
    return send(static_cast<SOCKET>(handle), reinterpret_cast<const char*>(buffer.data),
                static_cast<int>(buffer.size), 0);
}

bool wouldBlockOrTimeoutLastErrorNative()
{
    const int error = WSAGetLastError();
    return error == WSAEWOULDBLOCK || error == WSAETIMEDOUT || error == WSAEINPROGRESS;
}

} // namespace bltzr_socket_detail
