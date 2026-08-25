#include "trees/Octree.hpp"

#include <algorithm>
#include <cmath>

namespace blitzar_trees {

bool Octree::Cell::IsLeaf() const noexcept
{
    return std::all_of(children.begin(), children.end(),
        [](const std::size_t child) { return child == Cell::InvalidIndex; });
}

Octree::Octree(std::size_t max_particles, std::size_t max_cells, std::size_t leaf_capacity,
    std::size_t max_depth)
    : max_particles_(max_particles), max_cells_(max_cells), leaf_capacity_(leaf_capacity),
      max_depth_(max_depth), particle_count_(0), build_count_(0), refit_count_(0),
      indices_(max_particles), scratch_(max_particles), morton_keys_(max_particles)
{
    cells_.reserve(max_cells);
}

bool Octree::IsValidInput(blitzar_core::ParticleStateView particles) noexcept
{
    if (!blitzar_core::IsValid(particles)) {
        return false;
    }

    for (std::size_t index = 0; index < particles.SourceCount(); ++index) {
        if (!std::isfinite(particles.x[index]) || !std::isfinite(particles.y[index]) ||
            !std::isfinite(particles.z[index]) || !std::isfinite(particles.mass[index]) ||
            particles.mass[index] < 0.0) {
            return false;
        }
    }

    return true;
}

Octree::Cell Octree::MakeCell(CellPlacement placement) const noexcept
{
    Cell cell{};

    cell.center = placement.center;
    cell.half_extent = placement.half_extent;
    cell.begin = placement.begin;
    cell.count = placement.count;
    cell.depth = placement.depth;
    cell.children.fill(Cell::InvalidIndex);

    return cell;
}

std::size_t Octree::Octant(const Cell& cell, blitzar_core::Vector3 position) noexcept
{
    return (position.x >= cell.center.x ? std::size_t{1} : std::size_t{0}) |
           (position.y >= cell.center.y ? std::size_t{2} : std::size_t{0}) |
           (position.z >= cell.center.z ? std::size_t{4} : std::size_t{0});
}

bool Octree::Contains(const Cell& cell, blitzar_core::Vector3 position) noexcept
{
    return std::abs(position.x - cell.center.x) <= cell.half_extent &&
           std::abs(position.y - cell.center.y) <= cell.half_extent &&
           std::abs(position.z - cell.center.z) <= cell.half_extent;
}

} // namespace blitzar_trees
