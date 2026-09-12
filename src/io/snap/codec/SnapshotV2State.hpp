#ifndef BLITZAR_IO_SNAPSHOT_SNAPSHOT_V2_STATE_HPP
#define BLITZAR_IO_SNAPSHOT_SNAPSHOT_V2_STATE_HPP

#include "core/CoreExecution.hpp"
#include "core/CoreSnapshot.hpp"
#include "core/CoreUnits.hpp"

#include <cstdint>
#include <span>

namespace blitzar_io {

inline constexpr std::uint16_t SnapshotV2Version = 2;
inline constexpr std::size_t SnapshotV2HeaderBytes = 8;
inline constexpr std::size_t SnapshotV2ChecksumBytes = 8;
inline constexpr std::uint32_t SnapshotV2MaxSectionBytes = 1U << 28;

enum class SnapshotV2Section : std::uint16_t {
    ParticleState = 1,
    IntegratorState = 2,
    RngState = 3,
    UnitSystem = 4,
    ExecutionMath = 5,
    BackendIdentity = 6,
    GlobalDomain = 7,
    SourceOwnership = 8,
};

enum class SnapshotV2IntegratorKind : std::uint8_t { LeapfrogKdk = 0 };
enum class SnapshotV2BackendKind : std::uint8_t { Cpu = 0, Hip = 1 };
enum class SnapshotV2DeviceKind : std::uint8_t { HostCpu = 0, HipDevice = 1 };
enum class SnapshotV2PrecisionKind : std::uint8_t { Float64 = 0 };
enum class SnapshotV2CompensatorKind : std::uint8_t { DirectPlain = 0, BackendDefined = 1 };
enum class SnapshotV2OwnershipKind : std::uint8_t { PositionPartition = 0 };

struct SnapshotV2Particles final {
    std::span<const std::uint64_t> ids{};
    std::span<const blitzar_core::Scalar> position_x{};
    std::span<const blitzar_core::Scalar> position_y{};
    std::span<const blitzar_core::Scalar> position_z{};
    std::span<const blitzar_core::Scalar> velocity_x{};
    std::span<const blitzar_core::Scalar> velocity_y{};
    std::span<const blitzar_core::Scalar> velocity_z{};
    std::span<const blitzar_core::Scalar> mass{};

    [[nodiscard]] bool HasMatchingCounts(std::uint64_t count) const noexcept;
    [[nodiscard]] bool HasStrictlyIncreasingIds() const noexcept;
    [[nodiscard]] bool IsFinite() const noexcept;
};

struct SnapshotV2Integrator final {
    std::uint64_t step{};
    blitzar_core::Scalar time{};
    blitzar_core::Scalar timestep{};
    SnapshotV2IntegratorKind kind{SnapshotV2IntegratorKind::LeapfrogKdk};

    [[nodiscard]] bool IsValid() const noexcept;
};

struct SnapshotV2Rng final {
    std::uint64_t seed{};
};

struct SnapshotV2Units final {
    blitzar_core::UnitSystem units{};
};

struct SnapshotV2Math final {
    blitzar_core::ExecutionSettings settings{};
    SnapshotV2CompensatorKind compensator{SnapshotV2CompensatorKind::DirectPlain};

    [[nodiscard]] bool IsValid() const noexcept;
};

struct SnapshotV2Backend final {
    SnapshotV2BackendKind backend{SnapshotV2BackendKind::Cpu};
    SnapshotV2DeviceKind device{SnapshotV2DeviceKind::HostCpu};
    SnapshotV2PrecisionKind precision{SnapshotV2PrecisionKind::Float64};

    [[nodiscard]] bool IsValid() const noexcept;
};

struct SnapshotV2Domain final {
    blitzar_core::Vector3 minimum{};
    blitzar_core::Vector3 maximum{};
    std::uint32_t source_rank_count{1};
    std::uint32_t decomposition_version{1};

    [[nodiscard]] bool IsValid() const noexcept;
};

struct SnapshotV2Ownership final {
    SnapshotV2OwnershipKind kind{SnapshotV2OwnershipKind::PositionPartition};
    std::uint32_t source_rank_count{1};
    std::span<const std::uint32_t> source_rank{};

    [[nodiscard]] bool IsValid(std::uint64_t particle_count) const noexcept;
};

struct SnapshotV2State final {
    SnapshotV2Particles particles{};
    SnapshotV2Integrator integrator{};
    SnapshotV2Rng rng{};
    SnapshotV2Units units{};
    SnapshotV2Math math{};
    SnapshotV2Backend backend{};
    SnapshotV2Domain domain{};
    SnapshotV2Ownership ownership{};

    [[nodiscard]] std::uint64_t ParticleCount() const noexcept
    {
        return static_cast<std::uint64_t>(particles.ids.size());
    }

    [[nodiscard]] bool IsValid() const noexcept;
};

} // namespace blitzar_io

#endif
