#ifndef BLITZAR_MPI_PACKETS_MPI_PACKET_WIRE_HPP
#define BLITZAR_MPI_PACKETS_MPI_PACKET_WIRE_HPP

#include "core/CoreTypes.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <new>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace blitzar_parallel {

struct ParticlePacket final {
    std::uint64_t id{};
    blitzar_core::Scalar x{};
    blitzar_core::Scalar y{};
    blitzar_core::Scalar z{};
    blitzar_core::Scalar velocity_x{};
    blitzar_core::Scalar velocity_y{};
    blitzar_core::Scalar velocity_z{};
    blitzar_core::Scalar mass{};
};

static_assert(std::is_trivially_copyable_v<ParticlePacket>);
static_assert(std::is_standard_layout_v<ParticlePacket>);

struct AllToAllPacketRequest final {
    std::span<const ParticlePacket> send_packets;
    std::span<const int> send_counts;
    std::span<const int> send_displacements;
    std::span<ParticlePacket> receive_packets;
    std::span<const int> receive_counts;
    std::span<const int> receive_displacements;
};

inline constexpr std::size_t ParticleWireBytes = 64;
using ParticleWire = std::array<std::byte, ParticleWireBytes>;

class ParticleWireCodec final {
public:
    [[nodiscard]] static bool Encode(
        const ParticlePacket& packet, std::span<std::byte> output) noexcept;
    [[nodiscard]] static bool Decode(
        std::span<const std::byte> input, ParticlePacket& packet) noexcept;
    [[nodiscard]] static bool Encode(
        std::span<const ParticlePacket> packets, std::span<std::byte> output) noexcept;
    [[nodiscard]] static bool Decode(
        std::span<const std::byte> input, std::span<ParticlePacket> packets) noexcept;
};

class PacketBuffer final {
public:
    [[nodiscard]] std::size_t Size() const noexcept
    {
        return packets_.size();
    }

    void Clear() noexcept
    {
        packets_.clear();
    }

    void Reserve(std::size_t capacity)
    {
        packets_.reserve(capacity);
    }

    [[nodiscard]] bool EnsureCapacity(std::size_t capacity) noexcept
    {
        if (capacity <= packets_.capacity()) {
            return true;
        }

        try {
            packets_.reserve(capacity);
        }
        catch (const std::length_error&) {
            return false;
        }
        catch (const std::bad_alloc&) {
            return false;
        }

        return true;
    }

    void Resize(std::size_t size)
    {
        packets_.resize(size);
    }

    [[nodiscard]] bool ResizeBounded(std::size_t size) noexcept
    {
        if (size > packets_.capacity()) {
            return false;
        }

        packets_.resize(size);

        return true;
    }

    [[nodiscard]] std::size_t Capacity() const noexcept
    {
        return packets_.capacity();
    }

    [[nodiscard]] std::span<ParticlePacket> View() noexcept
    {
        return packets_;
    }

    [[nodiscard]] std::span<const ParticlePacket> View() const noexcept
    {
        return packets_;
    }

private:
    std::vector<ParticlePacket> packets_;
};

class ParticlePacker final {
public:
    [[nodiscard]] static bool Pack(blitzar_core::ParticleStateView state,
        std::span<const std::uint64_t> ids, std::span<ParticlePacket> output) noexcept
    {
        if (!blitzar_core::IsValid(state) || ids.size() != state.count ||
            output.size() < state.count) {
            return false;
        }

        for (std::size_t index = 0; index < state.count; ++index) {
            output[index] = ParticlePacket{ids[index], state.x[index], state.y[index],
                state.z[index], state.velocity_x[index], state.velocity_y[index],
                state.velocity_z[index], state.mass[index]};
        }

        return true;
    }
};

} // namespace blitzar_parallel

#endif
