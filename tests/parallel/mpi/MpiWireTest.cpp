#include "MpiCases.hpp"
#include "parallel/mpi/exchange/packets/PacketWire.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>

namespace blitzar_mpi_tests {

namespace {

bool CheckWireBytes(const blitzar_parallel::ParticleWire& wire, std::size_t offset,
    const std::array<unsigned int, 8>& expected) noexcept
{
    for (std::size_t index = 0; index < expected.size(); ++index) {
        if (wire[offset + index] != static_cast<std::byte>(expected[index])) {
            return false;
        }
    }

    return true;
}

bool CheckDecodedScalars(const blitzar_parallel::ParticlePacket& source,
    const blitzar_parallel::ParticlePacket& decoded) noexcept
{
    const std::array<double, 7> source_scalars{source.x, source.y, source.z, source.velocity_x,
        source.velocity_y, source.velocity_z, source.mass};

    const std::array<double, 7> decoded_scalars{decoded.x, decoded.y, decoded.z, decoded.velocity_x,
        decoded.velocity_y, decoded.velocity_z, decoded.mass};

    for (std::size_t index = 0; index < source_scalars.size(); ++index) {
        if (std::bit_cast<std::uint64_t>(source_scalars[index]) !=
            std::bit_cast<std::uint64_t>(decoded_scalars[index])) {
            return false;
        }
    }

    return true;
}

} // namespace

bool RunWireCodecCase() noexcept
{
    blitzar_parallel::PacketBuffer bounded_packets;

    bounded_packets.Reserve(2);

    if (!bounded_packets.ResizeBounded(2) || bounded_packets.ResizeBounded(3) ||
        bounded_packets.Size() != 2) {
        return false;
    }

    const blitzar_parallel::ParticlePacket source{
        0x0102030405060708ULL, 1.0, -2.5, 3.75, -4.5, 5.25, -6.75, 7.5};

    blitzar_parallel::ParticleWire wire{};

    if (!blitzar_parallel::ParticleWireCodec::Encode(source, wire)) {
        return false;
    }

    const std::array<unsigned int, 8> expected_id_bytes{
        0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01};

    const std::array<unsigned int, 8> expected_one_bytes{
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x3f};

    if (!CheckWireBytes(wire, 0, expected_id_bytes) ||
        !CheckWireBytes(wire, 8, expected_one_bytes)) {
        return false;
    }

    blitzar_parallel::ParticlePacket decoded{};

    if (!blitzar_parallel::ParticleWireCodec::Decode(wire, decoded) || decoded.id != source.id ||
        !CheckDecodedScalars(source, decoded)) {
        return false;
    }

    std::array<std::byte, blitzar_parallel::ParticleWireBytes - 1> short_wire{};

    return !blitzar_parallel::ParticleWireCodec::Encode(source, short_wire) &&
           !blitzar_parallel::ParticleWireCodec::Decode(short_wire, decoded);
}

} // namespace blitzar_mpi_tests
