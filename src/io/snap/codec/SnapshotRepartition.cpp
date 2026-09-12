#include "io/snap/codec/SnapshotRepartition.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>

namespace blitzar_io {

namespace {

enum class Axis : std::uint8_t { X = 0, Y = 1, Z = 2 };

[[nodiscard]] double Extent(
    const blitzar_core::Vector3& minimum, const blitzar_core::Vector3& maximum, Axis axis) noexcept
{
    switch (axis) {
    case Axis::X:

        return maximum.x - minimum.x;

    case Axis::Y:

        return maximum.y - minimum.y;

    case Axis::Z:

        return maximum.z - minimum.z;

    default:

        return 0.0;
    }
}

[[nodiscard]] double Component(const blitzar_core::Vector3& value, Axis axis) noexcept
{
    switch (axis) {
    case Axis::X:

        return value.x;

    case Axis::Y:

        return value.y;

    case Axis::Z:

        return value.z;

    default:

        return 0.0;
    }
}

[[nodiscard]] Axis LongestAxis(const SnapshotV2Domain& domain) noexcept
{
    const double x = Extent(domain.minimum, domain.maximum, Axis::X);
    const double y = Extent(domain.minimum, domain.maximum, Axis::Y);
    const double z = Extent(domain.minimum, domain.maximum, Axis::Z);

    if (x >= y && x >= z) {
        return Axis::X;
    }

    return y >= z ? Axis::Y : Axis::Z;
}

[[nodiscard]] std::size_t OwnerIndex(const blitzar_core::Vector3& position,
    const SnapshotV2Domain& domain, std::size_t rank_count) noexcept
{
    const Axis axis = LongestAxis(domain);
    const double span = Extent(domain.minimum, domain.maximum, axis);

    if (rank_count <= 1U || span <= 0.0) {
        return 0U;
    }

    const double origin = Component(domain.minimum, axis);
    const double offset = (Component(position, axis) - origin) / span;
    const double scaled = std::clamp(offset, 0.0, 1.0) * static_cast<double>(rank_count);

    return std::min(static_cast<std::size_t>(scaled), rank_count - 1U);
}

} // namespace

std::size_t SnapshotRepartition::RankCount(std::uint32_t destination_rank_count) const noexcept
{
    return static_cast<std::size_t>(destination_rank_count);
}

blitzar_status SnapshotRepartition::Assign(const SnapshotRepartitionRequest& request,
    std::span<const std::uint64_t> ids, std::span<const blitzar_core::Vector3> positions,
    const SnapshotRepartitionResult& result) const noexcept
{
    if (ids.size() != positions.size() || ids.size() != result.destination_rank.size() ||
        ids.size() > result.destination_order.size()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    if (request.destination_rank_count == 0 || !request.domain.IsValid()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    std::fill(result.destination_order.begin(), result.destination_order.end(), std::uint64_t{0});

    for (std::size_t index = 0; index < ids.size(); ++index) {
        const std::size_t rank = OwnerIndex(positions[index], request.domain,
            static_cast<std::size_t>(request.destination_rank_count));

        if (rank >= request.destination_rank_count) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }

        result.destination_rank[index] = static_cast<std::uint32_t>(rank);

        ++result.destination_order[rank];
    }

    return BLITZAR_STATUS_OK;
}

} // namespace blitzar_io
