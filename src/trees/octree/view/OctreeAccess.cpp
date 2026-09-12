#include "trees/octree/Octree.hpp"

namespace blitzar_trees {

std::size_t Octree::CellCount() const noexcept
{
    return cells_.size();
}

std::size_t Octree::ParticleCount() const noexcept
{
    return particle_count_;
}

std::size_t Octree::BuildCount() const noexcept
{
    return build_count_;
}

std::size_t Octree::RefitCount() const noexcept
{
    return refit_count_;
}

std::size_t Octree::MaxParticles() const noexcept
{
    return max_particles_;
}

std::size_t Octree::MaxCells() const noexcept
{
    return max_cells_;
}

std::uint64_t Octree::Generation() const noexcept
{
    return generation_;
}

std::span<const Octree::Cell> Octree::Cells() const noexcept
{
    return std::span<const Cell>(cells_);
}

std::span<const std::size_t> Octree::Indices() const noexcept
{
    if (particle_count_ == 0) {
        return {};
    }

    return std::span<const std::size_t>(indices_).first(particle_count_);
}

std::span<const Octree::Cell> Octree::CellAt(std::size_t index) const noexcept
{
    if (index >= cells_.size()) {
        return {};
    }

    return std::span<const Cell>(cells_).subspan(index, 1);
}

bool Octree::ParticleIndex(std::size_t index, std::size_t& particle) const noexcept
{
    if (index >= particle_count_) {
        return false;
    }

    particle = indices_[index];

    return particle < particle_count_;
}

} // namespace blitzar_trees
