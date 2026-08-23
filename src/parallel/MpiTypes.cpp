#include "parallel/MpiTypes.hpp"

#include <bit>
#include <limits>

namespace blitzar_parallel {

namespace {

template <typename T> [[nodiscard]] constexpr bool IsWireScalar() noexcept
{
    return sizeof(T) == sizeof(std::uint64_t) && std::numeric_limits<T>::is_iec559;
}

void WriteU64(std::uint64_t value, std::span<std::byte> output, std::size_t offset) noexcept
{
    for (std::size_t byte = 0; byte < sizeof(value); ++byte) {
        output[offset + byte] =
            static_cast<std::byte>((value >> (byte * 8U)) & std::uint64_t{0xff});
    }
}

[[nodiscard]] std::uint64_t ReadU64(std::span<const std::byte> input, std::size_t offset) noexcept
{
    std::uint64_t value = 0;

    for (std::size_t byte = 0; byte < sizeof(value); ++byte) {
        value |= static_cast<std::uint64_t>(std::to_integer<unsigned int>(input[offset + byte]))
                 << (byte * 8U);
    }

    return value;
}

template <typename T>
[[nodiscard]] bool WriteScalar(T value, std::span<std::byte> output, std::size_t offset) noexcept
{
    if constexpr (IsWireScalar<T>()) {
        WriteU64(std::bit_cast<std::uint64_t>(value), output, offset);

        return true;
    }

    (void)value;
    (void)output;
    (void)offset;

    return false;
}

template <typename T>
[[nodiscard]] bool ReadScalar(
    std::span<const std::byte> input, std::size_t offset, T& value) noexcept
{
    if constexpr (IsWireScalar<T>()) {
        value = std::bit_cast<T>(ReadU64(input, offset));

        return true;
    }
    (void)input;
    (void)offset;
    (void)value;
    return false;
}

} // namespace

bool ParticleWireCodec::Encode(const ParticlePacket& packet, std::span<std::byte> output) noexcept
{
    if (output.size() != ParticleWireBytes || !IsWireScalar<blitzar_core::Scalar>()) {
        return false;
    }

    WriteU64(packet.id, output, 0);

    return WriteScalar(packet.x, output, 8) && WriteScalar(packet.y, output, 16) &&
           WriteScalar(packet.z, output, 24) && WriteScalar(packet.velocity_x, output, 32) &&
           WriteScalar(packet.velocity_y, output, 40) &&
           WriteScalar(packet.velocity_z, output, 48) && WriteScalar(packet.mass, output, 56);
}

bool ParticleWireCodec::Decode(std::span<const std::byte> input, ParticlePacket& packet) noexcept
{
    if (input.size() != ParticleWireBytes || !IsWireScalar<blitzar_core::Scalar>()) {
        return false;
    }

    packet.id = ReadU64(input, 0);

    return ReadScalar(input, 8, packet.x) && ReadScalar(input, 16, packet.y) &&
           ReadScalar(input, 24, packet.z) && ReadScalar(input, 32, packet.velocity_x) &&
           ReadScalar(input, 40, packet.velocity_y) && ReadScalar(input, 48, packet.velocity_z) &&
           ReadScalar(input, 56, packet.mass);
}

bool ParticleWireCodec::Encode(
    std::span<const ParticlePacket> packets, std::span<std::byte> output) noexcept
{
    if (!IsWireScalar<blitzar_core::Scalar>() ||
        packets.size() > output.size() / ParticleWireBytes) {
        return false;
    }
    for (std::size_t index = 0; index < packets.size(); ++index) {
        if (!Encode(packets[index], output.subspan(index * ParticleWireBytes, ParticleWireBytes))) {
            return false;
        }
    }
    return true;
}

bool ParticleWireCodec::Decode(
    std::span<const std::byte> input, std::span<ParticlePacket> packets) noexcept
{
    if (!IsWireScalar<blitzar_core::Scalar>() ||
        packets.size() > input.size() / ParticleWireBytes) {
        return false;
    }
    for (std::size_t index = 0; index < packets.size(); ++index) {
        if (!Decode(input.subspan(index * ParticleWireBytes, ParticleWireBytes), packets[index])) {
            return false;
        }
    }
    return true;
}

} // namespace blitzar_parallel
