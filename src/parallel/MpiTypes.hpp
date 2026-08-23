#ifndef BLITZAR_PARALLEL_MPI_TYPES_HPP
#define BLITZAR_PARALLEL_MPI_TYPES_HPP

#include "core/Types.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
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

    [[nodiscard]] ParticlePacket* Data() noexcept
    {
        return packets_.data();
    }

    [[nodiscard]] const ParticlePacket* Data() const noexcept
    {
        return packets_.data();
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
