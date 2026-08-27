#include "solvers/barnes_hut/BhSolver.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <new>
#include <stdexcept>

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
    std::size_t local_particle_capacity)
    : settings_(settings), gravity_(gravity),
      local_particle_capacity_(
          local_particle_capacity == 0 ? settings.max_particles : local_particle_capacity),
      local_cell_capacity_(LocalCellCapacity(settings.max_cells,
          local_particle_capacity == 0 ? settings.max_particles : local_particle_capacity)),
      tree_(std::make_unique<blitzar_trees::Octree>(local_particle_capacity_, local_cell_capacity_,
          settings.leaf_capacity, settings.max_depth)),
      remote_tree_{}, stack_pool_(settings.max_cells, settings.max_depth),
      staging_(local_particle_capacity_)
{
}

blitzar_solvers::SolverKind BhSolver::Kind() const noexcept
{
    return blitzar_solvers::SolverKind::BarnesHut;
}

blitzar_status BhSolver::Prepare(std::size_t particle_capacity) noexcept
{
    return EnsureLocalCapacity(particle_capacity);
}

blitzar_status BhSolver::Compute(blitzar_core::ParticleStateView particles,
    blitzar_core::ForceView forces, const blitzar_core::ExecutionSettings& settings) noexcept
{
    if (!settings_.IsValid()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    return Compute(particles, forces, settings, stack_pool_);
}

blitzar_status BhSolver::Compute(blitzar_core::ParticleStateView particles,
    blitzar_core::ForceView forces, const blitzar_core::ExecutionSettings& settings,
    blitzar_solver_threading::ThreadStackPool& stack_pool) noexcept
{
    const blitzar_status prepare_status = Prepare(particles.count);

    if (prepare_status != BLITZAR_STATUS_OK) {
        return prepare_status;
    }

    const blitzar_status status =
        ComputeTree({*tree_, particles, particles, forces, settings, stack_pool, false, true});

    return status == BLITZAR_STATUS_OK ? CommitStagedForces(forces) : status;
}

blitzar_status BhSolver::ComputeSplit(const BarnesHutSplitRequest& request) noexcept
{
    return ComputeSplit(request, stack_pool_);
}

blitzar_status BhSolver::ComputeSplit(const BarnesHutSplitRequest& request,
    blitzar_solver_threading::ThreadStackPool& stack_pool) noexcept
{
    const blitzar_status prepare_status = Prepare(request.local.count);

    if (prepare_status != BLITZAR_STATUS_OK) {
        return prepare_status;
    }

    if (!settings_.IsValid() || request.remote.SourceCount() == 0) {
        const blitzar_status status = ComputeTree({*tree_, request.local, request.local,
            request.forces, request.settings, stack_pool, false, true});

        return status == BLITZAR_STATUS_OK ? CommitStagedForces(request.forces) : status;
    }

    const blitzar_status remote_tree_status = EnsureRemoteTree();

    if (remote_tree_status != BLITZAR_STATUS_OK) {
        return remote_tree_status;
    }

    const blitzar_status local_status = ComputeTree({*tree_, request.local, request.local,
        request.forces, request.settings, stack_pool, false, true});

    if (local_status != BLITZAR_STATUS_OK) {
        return local_status;
    }

    const blitzar_status remote_status = ComputeTree({*remote_tree_, request.local, request.remote,
        request.forces, request.settings, stack_pool, true, false});

    return remote_status == BLITZAR_STATUS_OK ? CommitStagedForces(request.forces) : remote_status;
}

blitzar_status BhSolver::EnsureRemoteTree() noexcept
{
    if (remote_tree_ != nullptr) {
        return BLITZAR_STATUS_OK;
    }

    try {
        remote_tree_ = std::make_unique<blitzar_trees::Octree>(settings_.max_particles,
            settings_.max_cells, settings_.leaf_capacity, settings_.max_depth);
    }
    catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }

    return BLITZAR_STATUS_OK;
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
    std::unique_ptr<blitzar_trees::Octree> candidate_tree;
    std::vector<blitzar_core::Vector3> candidate_staging;

    try {
        candidate_tree = std::make_unique<blitzar_trees::Octree>(
            particle_capacity, cell_capacity, settings_.leaf_capacity, settings_.max_depth);

        candidate_staging.resize(particle_capacity);
    }
    catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }

    tree_ = std::move(candidate_tree);
    staging_ = std::move(candidate_staging);
    local_particle_capacity_ = particle_capacity;
    local_cell_capacity_ = cell_capacity;

    return BLITZAR_STATUS_OK;
}

} // namespace blitzar_barnes_hut
