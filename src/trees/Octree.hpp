#ifndef BLITZAR_TREES_OCTREE_HPP
#define BLITZAR_TREES_OCTREE_HPP

#include "core/Types.hpp"

#include <blitzar/blitzar.h>

#include <array>
#include <cstddef>
#include <vector>

namespace blitzar_trees {

class Octree final {
public:
    struct Cell final {
        blitzar_core::Vector3 center{};
        blitzar_core::Vector3 center_of_mass{};
        blitzar_core::Scalar half_extent{};
        blitzar_core::Scalar mass{};
        std::size_t begin{};
        std::size_t count{};
        std::size_t depth{};
        std::array<int, 8> children{};

        [[nodiscard]] bool IsLeaf() const noexcept;
    };

    Octree(
        std::size_t max_particles,
        std::size_t max_cells,
        std::size_t leaf_capacity,
        std::size_t max_depth);

    [[nodiscard]] blitzar_status Build(
        blitzar_core::ParticleStateView particles) noexcept;
    [[nodiscard]] bool Refit(
        blitzar_core::ParticleStateView particles) noexcept;
    [[nodiscard]] std::size_t CellCount() const noexcept;
    [[nodiscard]] std::size_t ParticleCount() const noexcept;
    [[nodiscard]] std::size_t BuildCount() const noexcept;
    [[nodiscard]] std::size_t RefitCount() const noexcept;
    [[nodiscard]] const Cell& CellAt(std::size_t index) const noexcept;
    [[nodiscard]] std::size_t ParticleIndex(std::size_t index) const noexcept;

private:
    [[nodiscard]] static bool IsValidInput(
        blitzar_core::ParticleStateView particles) noexcept;
    [[nodiscard]] Cell MakeCell(
        blitzar_core::Vector3 center,
        blitzar_core::Scalar half_extent,
        std::size_t begin,
        std::size_t count,
        std::size_t depth) const noexcept;
    [[nodiscard]] static int Octant(
        const Cell& cell, blitzar_core::Vector3 position) noexcept;
    [[nodiscard]] static bool Contains(
        const Cell& cell, blitzar_core::Vector3 position) noexcept;
    void Partition(
        const Cell& cell,
        blitzar_core::ParticleStateView particles,
        std::array<std::size_t, 8>& counts) noexcept;
    void CalculateProperties(blitzar_core::ParticleStateView particles) noexcept;

    std::size_t max_particles_;
    std::size_t max_cells_;
    std::size_t leaf_capacity_;
    std::size_t max_depth_;
    std::size_t particle_count_;
    std::size_t build_count_;
    std::size_t refit_count_;
    std::vector<std::size_t> indices_;
    std::vector<std::size_t> scratch_;
    std::vector<Cell> cells_;
};

}  // namespace blitzar_trees

#endif
