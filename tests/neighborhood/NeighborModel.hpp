#ifndef BLITZAR_TESTS_NEIGHBORHOOD_NEIGHBOR_MODEL_HPP
#define BLITZAR_TESTS_NEIGHBORHOOD_NEIGHBOR_MODEL_HPP

#include "core/CoreTypes.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace blitzar_neighborhood {

enum class ScenarioKind : std::uint8_t { Dense, Sparse, Clustered, Moving };
enum class CandidateKind : std::uint8_t { CellLinked, SpatialHash, HilbertOrder, Verlet };
enum class GridKind : std::uint8_t { CellLinked, SpatialHash };

struct Bounds final {
    blitzar_core::Vector3 minimum{};
    blitzar_core::Vector3 maximum{};
};

struct NeighborParameters final {
    Bounds bounds{};
    blitzar_core::Scalar radius{};
    blitzar_core::Scalar skin{};
    std::size_t steps{};

    [[nodiscard]] bool IsValid() const noexcept;
};

struct NeighborWorkload final {
    std::uint64_t seed{};
    ScenarioKind scenario{};
    NeighborParameters parameters{};
    std::vector<blitzar_core::Vector3> positions{};
    std::vector<blitzar_core::Vector3> velocities{};
};

struct NeighborFrame final {
    explicit NeighborFrame(std::size_t count);

    [[nodiscard]] blitzar_core::ParticleStateView View() const noexcept;
    [[nodiscard]] blitzar_core::Vector3 Position(std::size_t index) const noexcept;

    std::vector<blitzar_core::Scalar> x{};
    std::vector<blitzar_core::Scalar> y{};
    std::vector<blitzar_core::Scalar> z{};
    std::vector<blitzar_core::Scalar> velocity_x{};
    std::vector<blitzar_core::Scalar> velocity_y{};
    std::vector<blitzar_core::Scalar> velocity_z{};
    std::vector<blitzar_core::Scalar> mass{};
};

struct NeighborSet final {
    std::vector<std::size_t> offsets{};
    std::vector<std::size_t> indices{};

    [[nodiscard]] bool IsValid(std::size_t particle_count) const noexcept;
    [[nodiscard]] std::size_t Count() const noexcept;
    [[nodiscard]] std::size_t MemoryBytes() const noexcept;
    [[nodiscard]] std::uint64_t Hash() const noexcept;
    [[nodiscard]] std::uint64_t OrderingHash() const noexcept;
};

struct TreeMetrics final {
    std::uint64_t build_ns{};
    std::size_t cells{};
    std::size_t memory_bytes{};
    std::uint64_t hash{};
    bool valid{};
};

struct CandidateRun final {
    std::uint64_t build_ns{};
    std::uint64_t query_ns{};
    std::size_t rebuild_count{};
    std::size_t neighbor_count{};
    std::size_t reference_count{};
    std::size_t memory_bytes{};
    std::uint64_t candidate_hash{};
    std::uint64_t reference_hash{};
    std::uint64_t ordering_hash{};
    bool finite{};
    bool correct{};
    bool repeatable{};
};

struct NeighborResult final {
    std::uint64_t seed{};
    ScenarioKind scenario{};
    CandidateKind candidate{};
    std::size_t particle_count{};
    std::size_t steps{};
    blitzar_core::Scalar radius{};
    blitzar_core::Scalar skin{};
    std::uint64_t build_ns{};
    std::uint64_t query_ns{};
    std::uint64_t total_ns{};
    std::size_t rebuild_count{};
    std::size_t neighbor_count{};
    std::size_t reference_count{};
    std::size_t memory_bytes{};
    std::uint64_t candidate_hash{};
    std::uint64_t reference_hash{};
    std::uint64_t ordering_hash{};
    std::uint64_t octree_build_ns{};
    std::size_t octree_cells{};
    std::size_t octree_memory_bytes{};
    std::uint64_t octree_hash{};
    bool finite{};
    bool correct{};
    bool repeatable{};
    bool selected{};
};

} // namespace blitzar_neighborhood

#endif
