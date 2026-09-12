#ifndef BLITZAR_IO_SNAPSHOT_SNAPSHOT_V2_READER_HPP
#define BLITZAR_IO_SNAPSHOT_SNAPSHOT_V2_READER_HPP

#include "io/snap/codec/SnapshotV2State.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

namespace blitzar_io {

struct SnapshotV2Target final {
    std::span<std::uint64_t> ids{};
    std::span<blitzar_core::Scalar> position_x{};
    std::span<blitzar_core::Scalar> position_y{};
    std::span<blitzar_core::Scalar> position_z{};
    std::span<blitzar_core::Scalar> velocity_x{};
    std::span<blitzar_core::Scalar> velocity_y{};
    std::span<blitzar_core::Scalar> velocity_z{};
    std::span<blitzar_core::Scalar> mass{};
    std::span<std::uint32_t> source_rank{};
    SnapshotV2Integrator integrator{};
    SnapshotV2Rng rng{};
    SnapshotV2Units units{};
    SnapshotV2Math math{};
    SnapshotV2Backend backend{};
    SnapshotV2Domain domain{};
    SnapshotV2OwnershipKind ownership_kind{SnapshotV2OwnershipKind::PositionPartition};
    std::uint32_t ownership_source_rank_count{};

    [[nodiscard]] bool HasCapacity(std::uint64_t particle_count) const noexcept
    {
        const std::size_t capacity = ids.size();

        return particle_count <= capacity && particle_count <= position_x.size() &&
               particle_count <= position_y.size() && particle_count <= position_z.size() &&
               particle_count <= velocity_x.size() && particle_count <= velocity_y.size() &&
               particle_count <= velocity_z.size() && particle_count <= mass.size() &&
               particle_count <= source_rank.size();
    }

    [[nodiscard]] bool IsValid(std::uint64_t particle_count) const noexcept
    {
        if (integrator.kind != SnapshotV2IntegratorKind::LeapfrogKdk ||
            integrator.timestep == 0.0 || !std::isfinite(integrator.time) ||
            !std::isfinite(integrator.timestep) || !math.settings.IsValid() ||
            !units.units.IsValid() || !backend.IsValid() || !domain.IsValid() ||
            ownership_kind != SnapshotV2OwnershipKind::PositionPartition ||
            ownership_source_rank_count == 0U ||
            domain.source_rank_count != ownership_source_rank_count) {
            return false;
        }

        const std::size_t size = static_cast<std::size_t>(particle_count);

        for (std::size_t index = 0; index < size; ++index) {
            if (source_rank[index] >= ownership_source_rank_count ||
                !std::isfinite(position_x[index]) || !std::isfinite(position_y[index]) ||
                !std::isfinite(position_z[index])) {
                return false;
            }

            if (index > 0U && ids[index] <= ids[index - 1U]) {
                return false;
            }
        }

        return true;
    }
};

class SnapshotV2Reader final {
public:
    explicit SnapshotV2Reader(std::size_t max_particle_count);

    [[nodiscard]] blitzar_status Read(const std::filesystem::path& path, SnapshotV2Target& target);

private:
    std::size_t max_particle_count_;
    std::vector<std::byte> buffer_;
};

} // namespace blitzar_io

#endif
