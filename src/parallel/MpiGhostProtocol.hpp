#ifndef BLITZAR_PARALLEL_MPI_GHOST_PROTOCOL_HPP
#define BLITZAR_PARALLEL_MPI_GHOST_PROTOCOL_HPP

#include "parallel/MpiTypes.hpp"

#include <blitzar/blitzar.h>
#include <climits>
#include <cstddef>
#include <limits>
#include <new>
#include <stdexcept>
#include <vector>

namespace blitzar_parallel {

class MpiGhostProtocol final {
public:
    inline static constexpr int DataTag = 7102;

    [[nodiscard]] static bool ToWireBytes(std::size_t packets, int& bytes) noexcept
    {
        if (packets > static_cast<std::size_t>(INT_MAX) / ParticleWireBytes) {
            return false;
        }

        bytes = static_cast<int>(packets * ParticleWireBytes);

        return true;
    }

    [[nodiscard]] static bool ToWireSize(std::size_t packets, std::size_t& bytes) noexcept
    {
        if (packets > std::numeric_limits<std::size_t>::max() / ParticleWireBytes) {
            return false;
        }

        bytes = packets * ParticleWireBytes;

        return true;
    }

    [[nodiscard]] static std::size_t PointChunkPackets() noexcept
    {
        return static_cast<std::size_t>(INT_MAX) / ParticleWireBytes;
    }

    [[nodiscard]] static std::size_t ChunkCount(std::size_t packets) noexcept
    {
        const std::size_t chunk_packets = PointChunkPackets();

        return packets == 0 ? 0 : 1 + (packets - 1) / chunk_packets;
    }

    template <typename Value>
    [[nodiscard]] static bool EnsureCapacity(
        std::vector<Value>& values, std::size_t capacity) noexcept
    {
        if (capacity <= values.capacity()) {
            return true;
        }

        try {
            values.reserve(capacity);
        }
        catch (const std::length_error&) {
            return false;
        }
        catch (const std::bad_alloc&) {
            return false;
        }

        return true;
    }

    template <typename Value>
    [[nodiscard]] static bool ResizeWithinCapacity(
        std::vector<Value>& values, std::size_t size) noexcept
    {
        if (size > values.capacity()) {
            return false;
        }

        values.resize(size);

        return true;
    }
};

} // namespace blitzar_parallel

#endif
