#ifndef BLITZAR_PARALLEL_PARTITION_HPP
#define BLITZAR_PARALLEL_PARTITION_HPP

#include "parallel/Bounds.hpp"
#include "parallel/MpiContext.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace blitzar_parallel {

class Partition final {
public:
    Partition() noexcept = default;

    [[nodiscard]] blitzar_status Initialize(blitzar_core::ParticleStateView state,
        DomainBounds global_bounds, const MpiContext& context) noexcept;
    [[nodiscard]] int Owner(blitzar_core::Vector3 position, std::uint64_t particle_id,
        DomainBounds global_bounds) const noexcept;

private:
    struct SplitKey final {
        std::uint64_t key{};
        std::uint64_t particle_id{};
    };

    [[nodiscard]] blitzar_status AllocateBuffers(
        std::size_t source_count, const MpiContext& context) noexcept;
    void BuildRootKeys(blitzar_core::ParticleStateView state, DomainBounds global_bounds) noexcept;
    void BuildRootSplits() noexcept;
    [[nodiscard]] blitzar_status BroadcastSplits(const MpiContext& context) noexcept;

    int size_{1};
    std::vector<SplitKey> split_keys_;
    std::vector<std::uint64_t> split_values_;
    std::vector<std::uint64_t> split_ids_;
    std::vector<std::uint64_t> keys_;
    std::vector<std::size_t> order_;
};

} // namespace blitzar_parallel

#endif
