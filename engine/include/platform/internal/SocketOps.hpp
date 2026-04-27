// File: engine/include/platform/internal/SocketOps.hpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#ifndef GRAVITY_ENGINE_INCLUDE_PLATFORM_INTERNAL_SOCKETOPS_HPP_
#define GRAVITY_ENGINE_INCLUDE_PLATFORM_INTERNAL_SOCKETOPS_HPP_
#include "platform/SocketPlatform.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
namespace grav_socket_detail {
/// Description: Defines the SocketAddressV4 data or behavior contract.
struct SocketAddressV4 {
    std::array<unsigned char, 4> addressBytes{0, 0, 0, 0};
    std::uint16_t port = 0u;
    bool anyAddress = false;
};
/// Description: Executes the invalidNativeSocket operation.
std::intptr_t invalidNativeSocket();
/// Description: Executes the initializeSocketLayer operation.
bool initializeSocketLayer();
/// Description: Executes the shutdownSocketLayer operation.
void shutdownSocketLayer();
/// Description: Executes the createTcpSocketNative operation.
std::intptr_t createTcpSocketNative();
/// Description: Executes the closeSocketNative operation.
void closeSocketNative(std::intptr_t handle);
/// Description: Executes the setReuseAddressNative operation.
bool setReuseAddressNative(std::intptr_t handle, bool enabled);
/// Description: Executes the setSocketTimeoutNative operation.
bool setSocketTimeoutNative(std::intptr_t handle, int timeoutMs);
/// Description: Executes the parseIpv4Address operation.
bool parseIpv4Address(const std::string& host, SocketAddressV4& outAddress);
/// Description: Executes the connectIpv4Native operation.
bool connectIpv4Native(std::intptr_t handle, const SocketAddressV4& address, int timeoutMs);
/// Description: Executes the bindIpv4Native operation.
bool bindIpv4Native(std::intptr_t handle, const SocketAddressV4& address);
/// Description: Executes the listenSocketNative operation.
bool listenSocketNative(std::intptr_t handle, int backlog);
/// Description: Executes the acceptSocketNative operation.
std::intptr_t acceptSocketNative(std::intptr_t handle);
/// Description: Executes the waitReadableNative operation.
bool waitReadableNative(std::intptr_t handle, int timeoutMs);
/// Description: Executes the recvBytesNative operation.
int recvBytesNative(std::intptr_t handle, grav_socket::MutableBytes buffer);
/// Description: Executes the sendBytesNative operation.
int sendBytesNative(std::intptr_t handle, grav_socket::ConstBytes buffer);
/// Description: Executes the wouldBlockOrTimeoutLastErrorNative operation.
bool wouldBlockOrTimeoutLastErrorNative();
} // namespace grav_socket_detail
#endif // GRAVITY_ENGINE_INCLUDE_PLATFORM_INTERNAL_SOCKETOPS_HPP_
