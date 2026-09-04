#include "solvers/fmm/kifmm/KifmmSolver.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <new>
#include <optional>
#include <stdexcept>
#include <utility>

namespace blitzar_kifmm {

bool KifmmSettings::IsValid() const noexcept
{
    return std::isfinite(opening_angle) && opening_angle >= 0.0 && max_particles > 0 &&
           max_cells > 0 && leaf_capacity > 0 && max_depth > 0;
}

KifmmSolver::KifmmSolver(blitzar_physics::GravityParameters gravity, KifmmSettings settings,
    std::size_t local_particle_capacity, blitzar_solvers::SolverTreeResources& resources)
    : settings_(settings), parameters_(gravity), gravity_(gravity),
      local_particle_capacity_(
          local_particle_capacity == 0 ? settings.max_particles : local_particle_capacity),
      local_cell_capacity_(LocalCellCapacity(settings.max_cells,
          local_particle_capacity == 0 ? settings.max_particles : local_particle_capacity)),
      resources_(resources),
      workspace_(settings.max_cells, settings.max_depth,
          local_particle_capacity == 0 ? settings.max_particles : local_particle_capacity)
{
}

void KifmmSolver::BindResources(blitzar_solvers::SolverTreeResources& resources) noexcept
{
    resources_ = resources;
}

blitzar_solvers::SolverKind KifmmSolver::Kind() const noexcept
{
    return blitzar_solvers::SolverKind::Kifmm;
}

blitzar_status KifmmSolver::Prepare(std::size_t particle_capacity) noexcept
{
    return EnsureLocalCapacity(particle_capacity);
}

blitzar_status KifmmSolver::Evaluate(
    const blitzar_solvers::SolverForceRequest::Tree& request) noexcept
{
    const TreeComputeRequest evaluation{request.resource, request.tree, request.targets,
        request.sources, request.forces, request.settings, request.accumulate, request.skip_self};

    const blitzar_status status = ComputeTree(evaluation);

    return status == BLITZAR_STATUS_OK ? CommitStagedForces(request.forces) : status;
}

blitzar_solvers::SolverTreeResources& KifmmSolver::Resources() noexcept
{
    return resources_.get();
}

const blitzar_solvers::SolverTreeResources& KifmmSolver::Resources() const noexcept
{
    return resources_.get();
}

std::size_t KifmmSolver::BuildCount() const noexcept
{
    return resources_.get().Local().BuildCount();
}

std::size_t KifmmSolver::RefitCount() const noexcept
{
    return resources_.get().Local().RefitCount();
}

std::size_t KifmmSolver::LocalCellCapacity(
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

blitzar_status KifmmSolver::EnsureLocalCapacity(std::size_t particle_capacity) noexcept
{
    if (particle_capacity <= local_particle_capacity_) {
        return BLITZAR_STATUS_OK;
    }
    if (!settings_.IsValid() || particle_capacity > settings_.max_particles) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const std::size_t cell_capacity = LocalCellCapacity(settings_.max_cells, particle_capacity);
    std::optional<blitzar_trees::OctreeResource> candidate_tree;
    std::optional<KifmmWorkspace> candidate_workspace;

    try {
        candidate_tree.emplace(blitzar_trees::OctreeResourceConfig{
            particle_capacity, cell_capacity, settings_.leaf_capacity, settings_.max_depth});

        candidate_workspace.emplace(settings_.max_cells, settings_.max_depth, particle_capacity);
    }
    catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }

    resources_.get().ReplaceLocal(std::move(*candidate_tree));

    workspace_ = std::move(*candidate_workspace);
    local_particle_capacity_ = particle_capacity;
    local_cell_capacity_ = cell_capacity;

    return BLITZAR_STATUS_OK;
}

bool KifmmSolver::IsValidState(blitzar_core::ParticleStateView particles) noexcept
{
    if (!blitzar_core::IsValid(particles)) {
        return false;
    }

    for (std::size_t index = 0; index < particles.SourceCount(); ++index) {
        if (!std::isfinite(particles.x[index]) || !std::isfinite(particles.y[index]) ||
            !std::isfinite(particles.z[index]) || !std::isfinite(particles.velocity_x[index]) ||
            !std::isfinite(particles.velocity_y[index]) ||
            !std::isfinite(particles.velocity_z[index]) || !std::isfinite(particles.mass[index]) ||
            particles.mass[index] < 0.0) {
            return false;
        }
    }

    return true;
}

blitzar_status KifmmSolver::PairStatusToStatus(blitzar_physics::PairStatus status) noexcept
{
    if (status == blitzar_physics::PairStatus::Singularity) {
        return BLITZAR_STATUS_SINGULARITY;
    }
    if (status == blitzar_physics::PairStatus::Invalid) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    return BLITZAR_STATUS_OK;
}

bool KifmmSolver::ValidateTreeRequest(const TreeComputeRequest& request) const noexcept
{
    const std::size_t source_count = request.sources.SourceCount();
    const std::span<const blitzar_trees::Octree::Cell> cells = request.tree.Cells();

    return settings_.IsValid() && gravity_.IsValid() && request.settings.IsValid() &&
           request.targets.count == request.forces.count && IsValidState(request.targets) &&
           IsValidState(request.sources) && blitzar_core::IsValid(request.forces) &&
           request.tree.IsValid() && request.resource.IsCurrent(request.tree) &&
           request.tree.ParticleCount() == source_count &&
           source_count <= request.resource.MaxParticles() &&
           cells.size() <= workspace_.CellCapacity() &&
           request.targets.count <= workspace_.TargetCapacity();
}

} // namespace blitzar_kifmm
