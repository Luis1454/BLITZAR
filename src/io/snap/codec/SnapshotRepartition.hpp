#ifndef BLITZAR_IO_SNAPSHOT_SNAPSHOT_REPARTITION_HPP
#define BLITZAR_IO_SNAPSHOT_SNAPSHOT_REPARTITION_HPP

#include "io/snap/codec/SnapshotV2State.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace blitzar_io {

struct SnapshotRepartitionRequest final {
    SnapshotV2Domain domain{};
    std::uint32_t destination_rank_count{1};
};

struct SnapshotRepartitionResult final {
    std::span<std::uint32_t> destination_rank{};
    std::span<std::uint64_t> destination_order{};
};

class SnapshotRepartition final {
public:
    SnapshotRepartition() noexcept = default;

    [[nodiscard]] blitzar_status Assign(const SnapshotRepartitionRequest& request,
        std::span<const std::uint64_t> ids, std::span<const blitzar_core::Vector3> positions,
        const SnapshotRepartitionResult& result) const noexcept;

    [[nodiscard]] std::size_t RankCount(std::uint32_t destination_rank_count) const noexcept;
};

} // namespace blitzar_io

#endif
