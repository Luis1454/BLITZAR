#ifndef BLITZAR_TESTS_BVH_BVH_MODEL_HPP
#define BLITZAR_TESTS_BVH_BVH_MODEL_HPP

#include "neighborhood/NeighborModel.hpp"

#include <cstddef>
#include <cstdint>

namespace blitzar_bvh {

inline constexpr std::size_t kLeafSize = 4;

struct BvhPassResult final {
    std::uint64_t build_ns{};
    std::uint64_t refit_ns{};
    std::uint64_t query_ns{};
    std::uint64_t rebuild_ns{};
    std::uint64_t cell_linked_build_ns{};
    std::uint64_t cell_linked_query_ns{};
    std::size_t rebuild_count{};
    std::size_t refit_count{};
    std::size_t rebuild_baseline_count{};
    std::size_t neighbor_count{};
    std::size_t reference_count{};
    std::size_t cell_linked_neighbor_count{};
    std::size_t bvh_memory_bytes{};
    std::size_t bvh_workspace_bytes{};
    std::size_t cell_linked_memory_bytes{};
    std::uint64_t bvh_hash{1469598103934665603ULL};
    std::uint64_t bvh_topology_hash{1469598103934665603ULL};
    std::uint64_t cell_linked_hash{1469598103934665603ULL};
    std::uint64_t reference_hash{1469598103934665603ULL};
    std::uint64_t bvh_ordering_hash{1469598103934665603ULL};
    std::uint64_t cell_linked_ordering_hash{1469598103934665603ULL};
    bool finite{true};
    bool correct{true};
    bool cell_linked_correct{true};
};

struct BvhScenarioResult final {
    std::uint64_t seed{};
    blitzar_neighborhood::ScenarioKind scenario{};
    std::size_t particle_count{};
    std::size_t steps{};
    blitzar_core::Scalar radius{};
    blitzar_core::Scalar skin{};
    std::size_t leaf_size{};
    std::uint64_t bvh_build_ns{};
    std::uint64_t bvh_refit_ns{};
    std::uint64_t bvh_query_ns{};
    std::uint64_t bvh_rebuild_ns{};
    std::uint64_t cell_linked_build_ns{};
    std::uint64_t cell_linked_query_ns{};
    std::size_t bvh_rebuild_count{};
    std::size_t bvh_refit_count{};
    std::size_t bvh_rebuild_baseline_count{};
    std::size_t bvh_neighbor_count{};
    std::size_t cell_linked_neighbor_count{};
    std::size_t reference_count{};
    std::size_t bvh_memory_bytes{};
    std::size_t bvh_workspace_bytes{};
    std::size_t cell_linked_memory_bytes{};
    std::uint64_t bvh_hash{};
    std::uint64_t bvh_topology_hash{};
    std::uint64_t cell_linked_hash{};
    std::uint64_t reference_hash{};
    std::uint64_t bvh_ordering_hash{};
    std::uint64_t cell_linked_ordering_hash{};
    std::uint64_t octree_build_ns{};
    std::size_t octree_cells{};
    std::size_t octree_memory_bytes{};
    std::uint64_t octree_hash{};
    bool finite{false};
    bool correct{false};
    bool repeatable{false};
    bool deterministic{false};
    bool refit_correct{false};
    bool rebuild_correct{false};
    bool refit_parity{false};
    bool selected{false};
};

} // namespace blitzar_bvh

#endif
