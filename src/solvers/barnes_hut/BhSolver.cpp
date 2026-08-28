#include "solvers/barnes_hut/BhSolver.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <new>
#include <optional>
#include <stdexcept>
#include <utility>

namespace blitzar_barnes_hut {

namespace {

[[nodiscard]] std::size_t LocalCellCapacity(
    std::size_t configured_cells, std::size_t particle_capacity) noexcept
{
    if (particle_capacity == 0 || configured_cells == 0) {
        return configured_cells;
    }

    const std::size_t maximum = std::numeric_limits<std::size_t>::max();
    const std::size_t particle_cells =
        particle_capacity > (maximum - 1) / 8 ? maximum : particle_capacity * 8 + 1;

    return std::min(configured_cells, particle_cells);
}

} // namespace

bool BarnesHutSettings::IsValid() const noexcept
{
    return std::isfinite(opening_angle) && opening_angle >= 0.0 && max_particles > 0 &&
           max_cells > 0 && leaf_capacity > 0 && max_depth > 0;
}

BhSolver::BhSolver(blitzar_physics::GravityParameters gravity, BarnesHutSettings settings,
    std::size_t local_particle_capacity, blitzar_solvers::SolverTreeResources& resources)
    : settings_(settings), gravity_(gravity),
      local_particle_capacity_(
          local_particle_capacity == 0 ? settings.max_particles : local_particle_capacity),
      local_cell_capacity_(LocalCellCapacity(settings.max_cells,
          local_particle_capacity == 0 ? settings.max_particles : local_particle_capacity)),
      resources_(resources), stack_pool_(settings.max_cells, settings.max_depth),
      staging_(local_particle_capacity_)
{
}

void BhSolver::BindResources(blitzar_solvers::SolverTreeResources& resources) noexcept
{
    resources_ = resources;
}

blitzar_solvers::SolverKind BhSolver::Kind() const noexcept
{
    return blitzar_solvers::SolverKind::BarnesHut;
}

blitzar_status BhSolver::Prepare(std::size_t particle_capacity) noexcept
{
    return EnsureLocalCapacity(particle_capacity);
}

blitzar_status BhSolver::Evaluate(const blitzar_solvers::SolverForceRequest::Tree& request) noexcept
{
    const TreeComputeRequest evaluation{request.resource, request.tree, request.targets,
        request.sources, request.forces, request.settings, stack_pool_, request.accumulate,
        request.skip_self};

    const blitzar_status status = ComputeTree(evaluation);

    return status == BLITZAR_STATUS_OK ? CommitStagedForces(request.forces) : status;
}

blitzar_solvers::SolverTreeResources& BhSolver::Resources() noexcept
{
    return resources_.get();
}

const blitzar_solvers::SolverTreeResources& BhSolver::Resources() const noexcept
{
    return resources_.get();
}

blitzar_status BhSolver::EnsureLocalCapacity(std::size_t particle_capacity) noexcept
{
    if (particle_capacity <= local_particle_capacity_) {
        return BLITZAR_STATUS_OK;
    }
    if (!settings_.IsValid() || particle_capacity > settings_.max_particles) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const std::size_t cell_capacity = LocalCellCapacity(settings_.max_cells, particle_capacity);
    std::optional<blitzar_trees::OctreeResource> candidate_tree;
    std::vector<blitzar_core::Vector3> candidate_staging;

    try {
        candidate_tree.emplace(blitzar_trees::OctreeResourceConfig{
            particle_capacity, cell_capacity, settings_.leaf_capacity, settings_.max_depth});

        candidate_staging.resize(particle_capacity);
    }
    catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }

    resources_.get().ReplaceLocal(std::move(*candidate_tree));

    staging_ = std::move(candidate_staging);
    local_particle_capacity_ = particle_capacity;
    local_cell_capacity_ = cell_capacity;

    return BLITZAR_STATUS_OK;
}

} // namespace blitzar_barnes_hut
