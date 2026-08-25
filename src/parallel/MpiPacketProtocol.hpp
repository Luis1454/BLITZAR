#ifndef BLITZAR_PARALLEL_MPI_PACKET_PROTOCOL_HPP
#define BLITZAR_PARALLEL_MPI_PACKET_PROTOCOL_HPP

#include "parallel/MpiCollectives.hpp"
#include "parallel/MpiTypes.hpp"

#include <blitzar/blitzar.h>
#include <climits>
#include <cstddef>
#include <limits>
#include <new>
#include <span>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace blitzar_parallel {

class MpiPacketProtocol final {
public:
    [[nodiscard]] static bool ValidateLayout(std::span<const int> counts,
        std::span<const int> displacements, std::size_t packet_count) noexcept
    {
        if (counts.size() != displacements.size()) {
            return false;
        }

        for (std::size_t index = 0; index < counts.size(); ++index) {
            if (counts[index] < 0 || displacements[index] < 0 ||
                static_cast<std::size_t>(displacements[index]) > packet_count ||
                static_cast<std::size_t>(counts[index]) >
                    packet_count - static_cast<std::size_t>(displacements[index])) {
                return false;
            }
        }

        return true;
    }

    [[nodiscard]] static blitzar_status SynchronizePreparation(const MpiCollectives& collectives,
        blitzar_status local_status, std::string_view phase) noexcept
    {
        blitzar_status global_status = BLITZAR_STATUS_INTERNAL_ERROR;
        const blitzar_status synchronization_status =
            collectives.SynchronizeStatus(local_status, "MpiPacketTransport", phase, global_status);

        return synchronization_status != BLITZAR_STATUS_OK ? synchronization_status : global_status;
    }

    [[nodiscard]] static bool ToWireBytes(std::size_t packets, int& bytes) noexcept
    {
        if (packets > static_cast<std::size_t>(INT_MAX) / ParticleWireBytes) {
            return false;
        }

        bytes = static_cast<int>(packets * ParticleWireBytes);

        return true;
    }

    [[nodiscard]] static bool ComputeRoundPacketLimit(
        int peer_count, std::size_t& packets_per_peer) noexcept
    {
        if (peer_count <= 0 || ParticleWireBytes == 0) {
            return false;
        }

        const std::size_t peers = static_cast<std::size_t>(peer_count);

        if (peers > std::numeric_limits<std::size_t>::max() / ParticleWireBytes) {
            return false;
        }

        const std::size_t bytes_per_round_packet = peers * ParticleWireBytes;

        packets_per_peer = static_cast<std::size_t>(INT_MAX) / bytes_per_round_packet;

        return packets_per_peer != 0;
    }

    [[nodiscard]] static bool HasRemaining(
        std::span<const int> counts, std::span<const std::size_t> progress) noexcept
    {
        for (std::size_t index = 0; index < counts.size(); ++index) {
            if (progress[index] < static_cast<std::size_t>(counts[index])) {
                return true;
            }
        }

        return false;
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

    template <typename Value>
    [[nodiscard]] static blitzar_status EnsureCapacity(
        std::vector<Value>& values, std::size_t capacity) noexcept
    {
        if (capacity <= values.capacity()) {
            return BLITZAR_STATUS_OK;
        }

        try {
            values.reserve(capacity);
        }
        catch (const std::length_error&) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
        catch (const std::bad_alloc&) {
            return BLITZAR_STATUS_ALLOCATION_FAILURE;
        }

        return BLITZAR_STATUS_OK;
    }
};

} // namespace blitzar_parallel

#endif
