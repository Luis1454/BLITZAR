#include "trees/octree/OctreeResource.hpp"

namespace blitzar_trees {

bool OctreeResourceConfig::IsValid() const noexcept
{
    return max_particles > 0 && max_cells > 0 && leaf_capacity > 0 && max_depth > 0;
}

OctreeResource::OctreeResource(OctreeResourceConfig config)
    : config_(config),
      tree_(config.max_particles, config.max_cells, config.leaf_capacity, config.max_depth)
{
}

blitzar_status OctreeResource::Prepare(blitzar_core::ParticleStateView particles) noexcept
{
    if (!config_.IsValid() || particles.SourceCount() > config_.max_particles) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    if (tree_.ParticleCount() == particles.SourceCount() && tree_.Generation() != 0 &&
        tree_.Refit(particles)) {
        return BLITZAR_STATUS_OK;
    }

    return tree_.Build(particles);
}

OctreeView OctreeResource::View() const noexcept
{
    return tree_.View();
}

bool OctreeResource::IsCurrent(OctreeView view) const noexcept
{
    return config_.IsValid() && view.MaxParticles() == config_.max_particles &&
           view.MaxCells() == config_.max_cells && tree_.IsCurrent(view);
}

std::size_t OctreeResource::MaxParticles() const noexcept
{
    return config_.max_particles;
}

std::size_t OctreeResource::MaxCells() const noexcept
{
    return config_.max_cells;
}

std::size_t OctreeResource::BuildCount() const noexcept
{
    return tree_.BuildCount();
}

std::size_t OctreeResource::RefitCount() const noexcept
{
    return tree_.RefitCount();
}

} // namespace blitzar_trees
