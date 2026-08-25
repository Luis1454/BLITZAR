#include "solvers/fmm/FmmSolver.hpp"

#include <algorithm>
#include <limits>
#include <new>
#include <stdexcept>

namespace blitzar_fmm {

FmmSolver::FmmSolver(blitzar_physics::GravityParameters gravity, FmmSettings settings,
    std::size_t local_particle_capacity)
    : settings_(settings), parameters_(gravity), gravity_(gravity),
      local_particle_capacity_(
          local_particle_capacity == 0 ? settings.max_particles : local_particle_capacity),
      local_cell_capacity_(LocalCellCapacity(settings.max_cells,
          local_particle_capacity == 0 ? settings.max_particles : local_particle_capacity)),
      tree_(std::make_unique<blitzar_trees::Octree>(local_particle_capacity_, local_cell_capacity_,
          settings.leaf_capacity, settings.max_depth)),
      remote_tree_(std::make_unique<blitzar_trees::Octree>(
          settings.max_particles, settings.max_cells, settings.leaf_capacity, settings.max_depth)),
      stack_pool_(settings.max_cells, settings.max_depth), multipoles_{}, remote_multipoles_{},
      staging_(local_particle_capacity_)
{
    multipoles_.reserve(local_cell_capacity_);
    remote_multipoles_.reserve(settings.max_cells);
}

blitzar_core::SolverKind FmmSolver::Kind() const noexcept
{
    return blitzar_core::SolverKind::Fmm;
}

blitzar_status FmmSolver::Prepare(std::size_t particle_capacity) noexcept
{
    return EnsureLocalCapacity(particle_capacity);
}

std::size_t FmmSolver::BuildCount() const noexcept
{
    return tree_ == nullptr ? 0 : tree_->BuildCount();
}

std::size_t FmmSolver::RefitCount() const noexcept
{
    return tree_ == nullptr ? 0 : tree_->RefitCount();
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

blitzar_status FmmSolver::Compute(blitzar_core::ParticleStateView particles,
    blitzar_core::ForceView forces, const blitzar_core::ExecutionSettings& settings) noexcept
{
    return Compute(particles, forces, settings, stack_pool_);
}

blitzar_status FmmSolver::Compute(blitzar_core::ParticleStateView particles,
    blitzar_core::ForceView forces, const blitzar_core::ExecutionSettings& settings,
    blitzar_barnes_hut::ThreadStackPool& stack_pool) noexcept
{
    const blitzar_status prepare_status = Prepare(particles.count);

    if (prepare_status != BLITZAR_STATUS_OK) {
        return prepare_status;
    }

    const blitzar_status status = ComputeTree(
        {*tree_, multipoles_, particles, particles, forces, settings, stack_pool, false, true});

    return status == BLITZAR_STATUS_OK ? CommitStagedForces(forces) : status;
}

blitzar_status FmmSolver::ComputeSplit(const FmmSplitRequest& request) noexcept
{
    return ComputeSplit(request, stack_pool_);
}

blitzar_status FmmSolver::ComputeSplit(
    const FmmSplitRequest& request, blitzar_barnes_hut::ThreadStackPool& stack_pool) noexcept
{
    const blitzar_status prepare_status = Prepare(request.local.count);

    if (prepare_status != BLITZAR_STATUS_OK) {
        return prepare_status;
    }

    const blitzar_status local_status = ComputeTree({*tree_, multipoles_, request.local,
        request.local, request.forces, request.settings, stack_pool, false, true});

    if (local_status != BLITZAR_STATUS_OK || request.remote.SourceCount() == 0) {
        return local_status == BLITZAR_STATUS_OK ? CommitStagedForces(request.forces)
                                                 : local_status;
    }

    const blitzar_status remote_status = ComputeTree({*remote_tree_, remote_multipoles_,
        request.local, request.remote, request.forces, request.settings, stack_pool, true, false});

    return remote_status == BLITZAR_STATUS_OK ? CommitStagedForces(request.forces) : remote_status;
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
    std::unique_ptr<blitzar_trees::Octree> candidate_tree;
    std::vector<Multipole> candidate_multipoles;
    std::vector<blitzar_core::Vector3> candidate_staging;

    try {
        candidate_tree = std::make_unique<blitzar_trees::Octree>(
            particle_capacity, cell_capacity, settings_.leaf_capacity, settings_.max_depth);

        candidate_multipoles.reserve(cell_capacity);
        candidate_staging.resize(particle_capacity);
    }
    catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }

    tree_ = std::move(candidate_tree);
    multipoles_ = std::move(candidate_multipoles);
    staging_ = std::move(candidate_staging);
    local_particle_capacity_ = particle_capacity;
    local_cell_capacity_ = cell_capacity;

    return BLITZAR_STATUS_OK;
}

} // namespace blitzar_fmm
