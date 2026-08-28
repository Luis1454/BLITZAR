#include "solvers/fmm/FmmSolver.hpp"

#include <algorithm>
#include <limits>
#include <new>
#include <optional>
#include <stdexcept>
#include <utility>

namespace blitzar_fmm {

FmmSolver::FmmSolver(blitzar_physics::GravityParameters gravity, FmmSettings settings,
    std::size_t local_particle_capacity, blitzar_solvers::SolverTreeResources& resources)
    : settings_(settings), parameters_(gravity), gravity_(gravity),
      local_particle_capacity_(
          local_particle_capacity == 0 ? settings.max_particles : local_particle_capacity),
      local_cell_capacity_(LocalCellCapacity(settings.max_cells,
          local_particle_capacity == 0 ? settings.max_particles : local_particle_capacity)),
      resources_(resources), stack_pool_(settings.max_cells, settings.max_depth), multipoles_{},
      remote_multipoles_{}, staging_(local_particle_capacity_)
{
    multipoles_.reserve(local_cell_capacity_);
    remote_multipoles_.reserve(settings.max_cells);
}

void FmmSolver::BindResources(blitzar_solvers::SolverTreeResources& resources) noexcept
{
    resources_ = resources;
}

blitzar_solvers::SolverKind FmmSolver::Kind() const noexcept
{
    return blitzar_solvers::SolverKind::Fmm;
}

blitzar_status FmmSolver::Prepare(std::size_t particle_capacity) noexcept
{
    return EnsureLocalCapacity(particle_capacity);
}

std::size_t FmmSolver::BuildCount() const noexcept
{
    return resources_.get().Local().BuildCount();
}

std::size_t FmmSolver::RefitCount() const noexcept
{
    return resources_.get().Local().RefitCount();
}

std::size_t FmmSolver::LocalCellCapacity(
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

blitzar_status FmmSolver::Evaluate(
    const blitzar_solvers::SolverForceRequest::Tree& request) noexcept
{
    std::vector<Multipole>& multipoles =
        request.source_kind == blitzar_solvers::SolverForceSourceKind::Local ? multipoles_
                                                                             : remote_multipoles_;

    const TreeComputeRequest evaluation{request.resource, request.tree, multipoles, request.targets,
        request.sources, request.forces, request.settings, stack_pool_, request.accumulate,
        request.skip_self};

    const blitzar_status status = ComputeTree(evaluation);

    return status == BLITZAR_STATUS_OK ? CommitStagedForces(request.forces) : status;
}

blitzar_solvers::SolverTreeResources& FmmSolver::Resources() noexcept
{
    return resources_.get();
}

const blitzar_solvers::SolverTreeResources& FmmSolver::Resources() const noexcept
{
    return resources_.get();
}

blitzar_status FmmSolver::EnsureLocalCapacity(std::size_t particle_capacity) noexcept
{
    if (particle_capacity <= local_particle_capacity_) {
        return BLITZAR_STATUS_OK;
    }
    if (!settings_.IsValid() || particle_capacity > settings_.max_particles) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const std::size_t cell_capacity = LocalCellCapacity(settings_.max_cells, particle_capacity);
    std::optional<blitzar_trees::OctreeResource> candidate_tree;
    std::vector<Multipole> candidate_multipoles;
    std::vector<blitzar_core::Vector3> candidate_staging;

    try {
        candidate_tree.emplace(blitzar_trees::OctreeResourceConfig{
            particle_capacity, cell_capacity, settings_.leaf_capacity, settings_.max_depth});

        candidate_multipoles.reserve(cell_capacity);
        candidate_staging.resize(particle_capacity);
    }
    catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }

    resources_.get().ReplaceLocal(std::move(*candidate_tree));

    multipoles_ = std::move(candidate_multipoles);
    staging_ = std::move(candidate_staging);
    local_particle_capacity_ = particle_capacity;
    local_cell_capacity_ = cell_capacity;

    return BLITZAR_STATUS_OK;
}

} // namespace blitzar_fmm
