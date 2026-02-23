#ifndef GRAVITY_PLATFORM_INTERNAL_SOCKETOPS_HPP
#define GRAVITY_PLATFORM_INTERNAL_SOCKETOPS_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

#include "sim/SocketPlatform.hpp"

namespace sim::socket::detail {

struct SocketAddressV4 {
    std::array<unsigned char, 4> addressBytes{0, 0, 0, 0};
    std::uint16_t port = 0u;
    bool anyAddress = false;
};

std::intptr_t invalidNativeSocket();
bool initializeSocketLayer();
void shutdownSocketLayer();
std::intptr_t createTcpSocketNative();
void closeSocketNative(std::intptr_t handle);
bool setReuseAddressNative(std::intptr_t handle, bool enabled);
bool setSocketTimeoutNative(std::intptr_t handle, int timeoutMs);
bool parseIpv4Address(const std::string &host, SocketAddressV4 &outAddress);
bool connectIpv4Native(std::intptr_t handle, const SocketAddressV4 &address, int timeoutMs);
bool bindIpv4Native(std::intptr_t handle, const SocketAddressV4 &address);
bool listenSocketNative(std::intptr_t handle, int backlog);
std::intptr_t acceptSocketNative(std::intptr_t handle);
bool waitReadableNative(std::intptr_t handle, int timeoutMs);
int recvBytesNative(std::intptr_t handle, MutableBytes buffer);
int sendBytesNative(std::intptr_t handle, ConstBytes buffer);
bool wouldBlockOrTimeoutLastErrorNative();

} // namespace sim::socket::detail

#endif // GRAVITY_PLATFORM_INTERNAL_SOCKETOPS_HPP
