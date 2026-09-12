#include "io/snap/codec/SnapshotV2State.hpp"

#include <cmath>
#include <cstddef>
#include <limits>

namespace blitzar_io {

namespace {

[[nodiscard]] bool IsFiniteSpan(std::span<const blitzar_core::Scalar> values) noexcept
{
    for (const blitzar_core::Scalar value : values) {
        if (!std::isfinite(value)) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] bool IsValidMassSpan(std::span<const blitzar_core::Scalar> values) noexcept
{
    for (const blitzar_core::Scalar value : values) {
        if (!std::isfinite(value) || value < 0.0) {
            return false;
        }
    }

    return true;
}

} // namespace

bool SnapshotV2Particles::HasMatchingCounts(std::uint64_t count) const noexcept
{
    if (count > std::numeric_limits<std::size_t>::max()) {
        return false;
    }

    const std::size_t size = static_cast<std::size_t>(count);

    return ids.size() == size && position_x.size() == size && position_y.size() == size &&
           position_z.size() == size && velocity_x.size() == size && velocity_y.size() == size &&
           velocity_z.size() == size && mass.size() == size;
}

bool SnapshotV2Particles::HasStrictlyIncreasingIds() const noexcept
{
    for (std::size_t index = 1; index < ids.size(); ++index) {
        if (ids[index] <= ids[index - 1U]) {
            return false;
        }
    }

    return true;
}

bool SnapshotV2Particles::IsFinite() const noexcept
{
    return IsFiniteSpan(position_x) && IsFiniteSpan(position_y) && IsFiniteSpan(position_z) &&
           IsFiniteSpan(velocity_x) && IsFiniteSpan(velocity_y) && IsFiniteSpan(velocity_z) &&
           IsValidMassSpan(mass);
}

bool SnapshotV2Integrator::IsValid() const noexcept
{
    return std::isfinite(time) && std::isfinite(timestep) && timestep != 0.0 &&
           kind == SnapshotV2IntegratorKind::LeapfrogKdk;
}

bool SnapshotV2Math::IsValid() const noexcept
{
    const bool valid_compensator = compensator == SnapshotV2CompensatorKind::DirectPlain ||
                                   compensator == SnapshotV2CompensatorKind::BackendDefined;

    return settings.IsValid() && valid_compensator;
}

bool SnapshotV2Backend::IsValid() const noexcept
{
    if (backend != SnapshotV2BackendKind::Cpu && backend != SnapshotV2BackendKind::Hip) {
        return false;
    }

    if (precision != SnapshotV2PrecisionKind::Float64) {
        return false;
    }

    switch (backend) {
    case SnapshotV2BackendKind::Cpu:

        return device == SnapshotV2DeviceKind::HostCpu;

    case SnapshotV2BackendKind::Hip:

        return device == SnapshotV2DeviceKind::HipDevice;

    default:

        return false;
    }
}

bool SnapshotV2Domain::IsValid() const noexcept
{
    const bool finite = std::isfinite(minimum.x) && std::isfinite(minimum.y) &&
                        std::isfinite(minimum.z) && std::isfinite(maximum.x) &&
                        std::isfinite(maximum.y) && std::isfinite(maximum.z);

    return finite && minimum.x <= maximum.x && minimum.y <= maximum.y && minimum.z <= maximum.z &&
           source_rank_count > 0 && decomposition_version > 0;
}

bool SnapshotV2Ownership::IsValid(std::uint64_t particle_count) const noexcept
{
    if (kind != SnapshotV2OwnershipKind::PositionPartition || source_rank_count == 0) {
        return false;
    }

    if (particle_count > std::numeric_limits<std::size_t>::max()) {
        return false;
    }

    if (source_rank.size() != static_cast<std::size_t>(particle_count)) {
        return false;
    }

    for (const std::uint32_t rank : source_rank) {
        if (rank >= source_rank_count) {
            return false;
        }
    }

    return true;
}

bool SnapshotV2State::IsValid() const noexcept
{
    const std::uint64_t count = ParticleCount();

    return particles.HasMatchingCounts(count) && particles.HasStrictlyIncreasingIds() &&
           particles.IsFinite() && integrator.IsValid() && units.units.IsValid() &&
           math.IsValid() && backend.IsValid() && domain.IsValid() &&
           domain.source_rank_count == ownership.source_rank_count && ownership.IsValid(count);
}

} // namespace blitzar_io
