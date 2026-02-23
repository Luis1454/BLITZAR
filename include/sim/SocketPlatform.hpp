#ifndef GRAVITY_SIM_SOCKETPLATFORM_HPP
#define GRAVITY_SIM_SOCKETPLATFORM_HPP

#include <cstddef>
#include <cstdint>
#include <string>

namespace sim::socket {

using Handle = std::intptr_t;

struct MutableBytes {
    std::byte *data = nullptr;
    std::size_t size = 0u;

    bool empty() const
    {
        return size == 0u;
    }

    MutableBytes subview(std::size_t offset) const
    {
        if (offset >= size) {
            return {};
        }
        return MutableBytes{data + offset, size - offset};
    }
};

struct ConstBytes {
    const std::byte *data = nullptr;
    std::size_t size = 0u;

    bool empty() const
    {
        return size == 0u;
    }

    ConstBytes subview(std::size_t offset) const
    {
        if (offset >= size) {
            return {};
        }
        return ConstBytes{data + offset, size - offset};
    }
};

Handle invalidHandle();
bool isValid(Handle handle);
int clampTimeoutMs(int timeoutMs);

bool initializeSocketLayer();
void shutdownSocketLayer();

Handle createTcpSocket();
void closeSocket(Handle handle);
bool setReuseAddress(Handle handle, bool enabled);
bool setSocketTimeoutMs(Handle handle, int timeoutMs);
bool connectIpv4(Handle handle, const std::string &host, std::uint16_t port);
bool connectIpv4(Handle handle, const std::string &host, std::uint16_t port, int timeoutMs);
bool bindIpv4(Handle handle, const std::string &bindAddress, std::uint16_t port);
bool listenSocket(Handle handle, int backlog);
Handle acceptSocket(Handle listenHandle);
bool waitReadable(Handle handle, int timeoutMs);
int recvBytes(Handle handle, MutableBytes buffer);
int sendBytes(Handle handle, ConstBytes buffer);
bool wouldBlockOrTimeoutLastError();

} // namespace sim::socket

#endif // GRAVITY_SIM_SOCKETPLATFORM_HPP
