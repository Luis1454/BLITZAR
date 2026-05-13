#include "platform/internal/SocketOps.hpp"
#include <arpa/inet.h>
#include <cerrno>
#include <sys/socket.h>
#include <cstring>
#include <unistd.h>

namespace bltzr_socket_detail {

sockaddr_in makeSockaddr(const bltzr_socket_detail::SocketAddressV4& address)
{
    sockaddr_in nativeAddress{};
    nativeAddress.sin_family = AF_INET;
    nativeAddress.sin_port = htons(address.port);
    const std::uint32_t hostAddress =
        (static_cast<std::uint32_t>(address.addressBytes[0]) << 24u) |
        (static_cast<std::uint32_t>(address.addressBytes[1]) << 16u) |
        (static_cast<std::uint32_t>(address.addressBytes[2]) << 8u) |
        static_cast<std::uint32_t>(address.addressBytes[3]);
    nativeAddress.sin_addr.s_addr = address.anyAddress ? htonl(INADDR_ANY) : htonl(hostAddress);
    return nativeAddress;
}

std::intptr_t invalidNativeSocket()
{
    return -1;
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
    const int socketHandle = ::socket(AF_INET, SOCK_STREAM, 0);
    return socketHandle < 0 ? -1 : static_cast<std::intptr_t>(socketHandle);
}

void closeSocketNative(std::intptr_t handle)
{
    if (handle != invalidNativeSocket()) {
        ::close(static_cast<int>(handle));
    }
}

bool setReuseAddressNative(std::intptr_t handle, bool enabled)
{
    const int option = enabled ? 1 : 0;
    return ::setsockopt(static_cast<int>(handle), SOL_SOCKET, SO_REUSEADDR, &option,
                        sizeof(option)) == 0;
}

bool setSocketTimeoutNative(std::intptr_t handle, int timeoutMs)
{
    if (timeoutMs < 0) {
        return true;
    }
    timeval timeout{};
    timeout.tv_sec = timeoutMs / 1000;
    timeout.tv_usec = (timeoutMs % 1000) * 1000;
    return ::setsockopt(static_cast<int>(handle), SOL_SOCKET, SO_RCVTIMEO, &timeout,
                        sizeof(timeout)) == 0;
}

bool parseIpv4Address(const std::string& host, SocketAddressV4& outAddress)
{
    if (host.empty() || host == "0.0.0.0") {
        outAddress.addressBytes = {0, 0, 0, 0};
        outAddress.anyAddress = true;
        return true;
    }
    in_addr nativeAddress{};
    if (::inet_pton(AF_INET, host.c_str(), &nativeAddress) != 1) {
        return false;
    }
    const std::uint32_t hostAddress = ntohl(nativeAddress.s_addr);
    outAddress.addressBytes[0] = static_cast<unsigned char>((hostAddress >> 24u) & 0xFFu);
    outAddress.addressBytes[1] = static_cast<unsigned char>((hostAddress >> 16u) & 0xFFu);
    outAddress.addressBytes[2] = static_cast<unsigned char>((hostAddress >> 8u) & 0xFFu);
    outAddress.addressBytes[3] = static_cast<unsigned char>(hostAddress & 0xFFu);
    outAddress.anyAddress = false;
    return true;
}

bool connectIpv4Native(std::intptr_t handle, const SocketAddressV4& address, int timeoutMs)
{
    (void)timeoutMs;
    sockaddr_in nativeAddress = makeSockaddr(address);
    return ::connect(static_cast<int>(handle),
                     reinterpret_cast<const sockaddr*>(&nativeAddress),
                     sizeof(nativeAddress)) == 0;
}

bool bindIpv4Native(std::intptr_t handle, const SocketAddressV4& address)
{
    sockaddr_in nativeAddress = makeSockaddr(address);
    return ::bind(static_cast<int>(handle), reinterpret_cast<const sockaddr*>(&nativeAddress),
                  sizeof(nativeAddress)) == 0;
}

bool listenSocketNative(std::intptr_t handle, int backlog)
{
    return ::listen(static_cast<int>(handle), backlog) == 0;
}

std::intptr_t acceptSocketNative(std::intptr_t handle)
{
    const int socketHandle = ::accept(static_cast<int>(handle), nullptr, nullptr);
    return socketHandle < 0 ? -1 : static_cast<std::intptr_t>(socketHandle);
}

bool waitReadableNative(std::intptr_t handle, int timeoutMs)
{
    fd_set set;
    FD_ZERO(&set);
    FD_SET(static_cast<int>(handle), &set);
    timeval timeout{};
    timeval* timeoutPtr = nullptr;
    if (timeoutMs >= 0) {
        timeout.tv_sec = timeoutMs / 1000;
        timeout.tv_usec = (timeoutMs % 1000) * 1000;
        timeoutPtr = &timeout;
    }
    return ::select(static_cast<int>(handle) + 1, &set, nullptr, nullptr, timeoutPtr) > 0;
}

int recvBytesNative(std::intptr_t handle, bltzr_socket::MutableBytes buffer)
{
    return static_cast<int>(
        ::recv(static_cast<int>(handle), reinterpret_cast<void*>(buffer.data), buffer.size, 0));
}

int sendBytesNative(std::intptr_t handle, bltzr_socket::ConstBytes buffer)
{
    return static_cast<int>(::send(static_cast<int>(handle),
                                   reinterpret_cast<const void*>(buffer.data), buffer.size, 0));
}

bool wouldBlockOrTimeoutLastErrorNative()
{
    return errno == EAGAIN || errno == EWOULDBLOCK || errno == ETIMEDOUT ||
           errno == EINPROGRESS;
}

} // namespace bltzr_socket_detail
