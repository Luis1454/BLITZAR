#include "trees/octree/Octree.hpp"

#include <memory>

namespace blitzar_trees {

OctreeView Octree::View() const noexcept
{
    OctreeView view{};

    view.cells_ = Cells();
    view.indices_ = Indices();
    view.particle_count_ = particle_count_;
    view.max_particles_ = max_particles_;
    view.max_cells_ = max_cells_;
    view.generation_ = generation_;
    view.owner_ = std::cref(*this);

    return view;
}

bool Octree::IsCurrent(OctreeView view) const noexcept
{
    return view.IsValid() && view.owner_.has_value() && &view.owner_->get() == this &&
           view.generation_ == generation_ && view.particle_count_ == particle_count_ &&
           view.max_particles_ == max_particles_ && view.max_cells_ == max_cells_ &&
           view.cells_.size() == cells_.size() && view.indices_.size() == particle_count_;
}

bool OctreeView::IsValid() const noexcept
{
    return owner_.has_value() && generation_ != 0 && max_particles_ > 0 && max_cells_ > 0 &&
           particle_count_ <= max_particles_ && cells_.size() <= max_cells_ &&
           indices_.size() == particle_count_;
}

std::size_t OctreeView::ParticleCount() const noexcept
{
    return particle_count_;
}

std::size_t OctreeView::MaxParticles() const noexcept
{
    return max_particles_;
}

std::size_t OctreeView::MaxCells() const noexcept
{
    return max_cells_;
}

std::uint64_t OctreeView::Generation() const noexcept
{
    return generation_;
}

std::span<const Octree::Cell> OctreeView::Cells() const noexcept
{
    return cells_;
}

std::span<const std::size_t> OctreeView::Indices() const noexcept
{
    return indices_;
}

std::span<const Octree::Cell> OctreeView::CellAt(std::size_t index) const noexcept
{
    if (index >= cells_.size()) {
        return {};
    }

    return cells_.subspan(index, 1);
}

bool OctreeView::ParticleIndex(std::size_t index, std::size_t& particle) const noexcept
{
    if (index >= indices_.size()) {
        return false;
    }

    particle = indices_[index];

    return particle < particle_count_;
}

} // namespace blitzar_trees
