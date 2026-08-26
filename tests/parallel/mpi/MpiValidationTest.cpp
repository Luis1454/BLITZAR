#include "MpiCases.hpp"
#include "parallel/mpi/exchange/packets/PacketWire.hpp"

#include <array>
#include <cstddef>
#include <limits>
#include <span>

namespace blitzar_mpi_tests {

bool RunNestedContextCase(const blitzar_parallel::MpiContext& outer) noexcept
{
    blitzar_parallel::MpiContext nested;

    return nested.IsUsable() && nested.Rank() == outer.Rank() && nested.Size() == outer.Size();
}

bool RunCollectiveValidationCase(const blitzar_parallel::MpiContext& context) noexcept
{
    const std::array<int, 1> invalid_counts{0};
    std::array<int, 1> invalid_receive{};
    const blitzar_status expected_zero_layout =
        context.IsDistributed() ? BLITZAR_STATUS_INVALID_ARGUMENT : BLITZAR_STATUS_OK;

    if (context.AllToAllCounts(invalid_counts, invalid_receive) != expected_zero_layout ||
        context.AllGatherCounts(0, invalid_receive) != expected_zero_layout) {
        return false;
    }

    const std::array<blitzar_parallel::ParticlePacket, 0> empty_packets{};
    const blitzar_parallel::AllToAllPacketRequest invalid_request{empty_packets, invalid_counts,
        invalid_counts, std::span<blitzar_parallel::ParticlePacket>{}, invalid_counts,
        invalid_counts};

    if (context.AllToAllPackets(invalid_request) != expected_zero_layout) {
        return false;
    }

    const std::array<double, 2> invalid_minimum{};
    const std::array<double, 3> invalid_maximum{};
    std::array<double, 2> minimum = invalid_minimum;
    std::array<double, 3> maximum = invalid_maximum;

    return context.ReduceBounds(minimum, maximum) == BLITZAR_STATUS_INVALID_ARGUMENT;
}

bool RunLargeCountValidationCase(const blitzar_parallel::MpiContext& context) noexcept
{
    std::array<int, 4> counts{};
    std::array<int, 4> displacements{};

    counts.fill(std::numeric_limits<int>::max());

    const std::span<const int> layout =
        std::span<const int>(counts).first(static_cast<std::size_t>(context.Size()));

    const std::span<const int> offsets =
        std::span<const int>(displacements).first(static_cast<std::size_t>(context.Size()));

    const std::span<blitzar_parallel::ParticlePacket> empty_packets{};
    const blitzar_parallel::AllToAllPacketRequest request{
        empty_packets, layout, offsets, empty_packets, layout, offsets};

    return context.AllToAllPackets(request) == BLITZAR_STATUS_INVALID_ARGUMENT;
}

} // namespace blitzar_mpi_tests
