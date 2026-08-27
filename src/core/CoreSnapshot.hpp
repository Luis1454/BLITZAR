#ifndef BLITZAR_CORE_CORE_SNAPSHOT_HPP
#define BLITZAR_CORE_CORE_SNAPSHOT_HPP

#include "core/CoreStatus.hpp"
#include "core/CoreTypes.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <span>

namespace blitzar_core {

inline constexpr std::uint32_t SnapshotMagic = 0x425A5253U;
inline constexpr std::uint16_t SnapshotScalarBytes = sizeof(std::uint64_t);
inline constexpr std::uint64_t SnapshotMaxParticleCount = 100000;

static_assert(sizeof(Scalar) == SnapshotScalarBytes);
static_assert(std::numeric_limits<Scalar>::is_iec559);
static_assert(std::numeric_limits<Scalar>::digits == 53);

enum class SnapshotVersion : std::uint16_t { V1 = 1 };
enum class SnapshotDistribution : std::uint8_t { SingleRank = 0, Sharded = 1 };
enum class SnapshotIdPolicy : std::uint8_t { GlobalContiguous = 0, GlobalStable = 1 };

enum class SnapshotHeaderField : std::uint8_t {
    Magic = 0,
    Version = 1,
    ScalarBytes = 2,
    ParticleCount = 3,
    Step = 4,
    Time = 5,
    RankCount = 6,
    RankIndex = 7,
    Distribution = 8,
    IdPolicy = 9
};

inline constexpr std::array<SnapshotHeaderField, 10> SnapshotHeaderFieldOrder{
    SnapshotHeaderField::Magic, SnapshotHeaderField::Version, SnapshotHeaderField::ScalarBytes,
    SnapshotHeaderField::ParticleCount, SnapshotHeaderField::Step, SnapshotHeaderField::Time,
    SnapshotHeaderField::RankCount, SnapshotHeaderField::RankIndex,
    SnapshotHeaderField::Distribution, SnapshotHeaderField::IdPolicy};

enum class SnapshotField : std::uint8_t {
    Ids = 0,
    PositionX = 1,
    PositionY = 2,
    PositionZ = 3,
    VelocityX = 4,
    VelocityY = 5,
    VelocityZ = 6,
    Mass = 7
};

inline constexpr std::array<SnapshotField, 8> SnapshotPayloadOrder{SnapshotField::Ids,
    SnapshotField::PositionX, SnapshotField::PositionY, SnapshotField::PositionZ,
    SnapshotField::VelocityX, SnapshotField::VelocityY, SnapshotField::VelocityZ,
    SnapshotField::Mass};

struct SnapshotHeader final {
    std::uint32_t magic{SnapshotMagic};
    SnapshotVersion version{SnapshotVersion::V1};
    std::uint16_t scalar_bytes{SnapshotScalarBytes};
    std::uint64_t particle_count{};
    std::uint64_t step{};
    Scalar time{};
    std::uint32_t rank_count{1};
    std::uint32_t rank_index{};
    SnapshotDistribution distribution{SnapshotDistribution::SingleRank};
    SnapshotIdPolicy id_policy{SnapshotIdPolicy::GlobalContiguous};

    [[nodiscard]] blitzar_status Validate() const noexcept
    {
        if (magic != SnapshotMagic || particle_count > SnapshotMaxParticleCount ||
            !std::isfinite(time) || rank_count == 0 || rank_index >= rank_count) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        if (version != SnapshotVersion::V1 || scalar_bytes != SnapshotScalarBytes) {
            return BLITZAR_STATUS_UNSUPPORTED;
        }

        switch (distribution) {
        case SnapshotDistribution::SingleRank:

            if (rank_count != 1 || rank_index != 0 ||
                id_policy != SnapshotIdPolicy::GlobalContiguous) {
                return BLITZAR_STATUS_INVALID_ARGUMENT;
            }

            return BLITZAR_STATUS_OK;

        case SnapshotDistribution::Sharded:

            if (id_policy != SnapshotIdPolicy::GlobalStable) {
                return BLITZAR_STATUS_INVALID_ARGUMENT;
            }

            return BLITZAR_STATUS_UNSUPPORTED;

        default:

            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
    }

    [[nodiscard]] bool IsCompatible() const noexcept
    {
        return Validate() == BLITZAR_STATUS_OK;
    }
};

[[nodiscard]] inline bool IsFiniteSnapshotSpan(std::span<const Scalar> values) noexcept
{
    for (const Scalar value : values) {
        if (!std::isfinite(value)) {
            return false;
        }
    }

    return true;
}

struct SnapshotPayloadView final {
    std::span<const std::uint64_t> ids{};
    std::span<const Scalar> position_x{};
    std::span<const Scalar> position_y{};
    std::span<const Scalar> position_z{};
    std::span<const Scalar> velocity_x{};
    std::span<const Scalar> velocity_y{};
    std::span<const Scalar> velocity_z{};
    std::span<const Scalar> mass{};

    [[nodiscard]] bool HasMatchingCounts(std::uint64_t expected_count) const noexcept
    {
        if (expected_count > std::numeric_limits<std::size_t>::max()) {
            return false;
        }

        const std::size_t count = static_cast<std::size_t>(expected_count);

        return ids.size() == count && position_x.size() == count && position_y.size() == count &&
               position_z.size() == count && velocity_x.size() == count &&
               velocity_y.size() == count && velocity_z.size() == count && mass.size() == count;
    }

    [[nodiscard]] bool HasContiguousIds() const noexcept
    {
        for (std::size_t index = 0; index < ids.size(); ++index) {
            if (ids[index] != index) {
                return false;
            }
        }

        return true;
    }

    [[nodiscard]] bool IsFinite() const noexcept
    {
        return IsFiniteSnapshotSpan(position_x) && IsFiniteSnapshotSpan(position_y) &&
               IsFiniteSnapshotSpan(position_z) && IsFiniteSnapshotSpan(velocity_x) &&
               IsFiniteSnapshotSpan(velocity_y) && IsFiniteSnapshotSpan(velocity_z) &&
               IsFiniteSnapshotSpan(mass);
    }
};

struct SnapshotFrameView final {
    SnapshotHeader header{};
    SnapshotPayloadView payload{};

    [[nodiscard]] blitzar_status Validate() const noexcept
    {
        const blitzar_status header_status = header.Validate();

        if (header_status != BLITZAR_STATUS_OK) {
            return header_status;
        }

        if (!payload.HasMatchingCounts(header.particle_count) || !payload.HasContiguousIds() ||
            !payload.IsFinite()) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        return BLITZAR_STATUS_OK;
    }
};

} // namespace blitzar_core

#endif
