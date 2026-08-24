#include "solvers/barnes_hut/BarnesHutSolver.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
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

struct BarnesHutSolver::AccumulationRequest final {
    const blitzar_trees::Octree& tree;
    blitzar_core::ParticleStateView targets;
    blitzar_core::ParticleStateView sources;
    std::size_t target{};
    std::span<std::size_t> stack;
    blitzar_core::Vector3& acceleration;
    bool skip_self{false};
};

struct BarnesHutSolver::TreeComputeRequest final {
    blitzar_trees::Octree& tree;
    blitzar_core::ParticleStateView targets;
    blitzar_core::ParticleStateView sources;
    blitzar_core::ForceView forces;
    blitzar_core::ExecutionSettings settings;
    ThreadWorkspace& workspace;
    bool accumulate{false};
    bool skip_self{false};
};

bool BarnesHutSettings::IsValid() const noexcept
{
    return std::isfinite(opening_angle) && opening_angle >= 0.0 && max_particles > 0 &&
           max_cells > 0 && leaf_capacity > 0 && max_depth > 0;
}

BarnesHutSolver::BarnesHutSolver(blitzar_physics::GravityParameters gravity,
    BarnesHutSettings settings, std::size_t local_particle_capacity)
    : settings_(settings), gravity_(gravity),
      local_particle_capacity_(local_particle_capacity == 0 ? settings.max_particles
                                                             : local_particle_capacity),
      local_cell_capacity_(LocalCellCapacity(settings.max_cells,
          local_particle_capacity == 0 ? settings.max_particles : local_particle_capacity)),
      tree_(std::make_unique<blitzar_trees::Octree>(local_particle_capacity_, local_cell_capacity_,
          settings.leaf_capacity, settings.max_depth)),
      remote_tree_{},
      workspace_(settings.max_cells, settings.max_depth), staging_(local_particle_capacity_)
{
}

blitzar_core::SolverKind BarnesHutSolver::Kind() const noexcept
{
    return blitzar_core::SolverKind::BarnesHut;
}

blitzar_status BarnesHutSolver::Prepare(std::size_t particle_capacity) noexcept
{
    return EnsureLocalCapacity(particle_capacity);
}

bool BarnesHutSolver::IsValidState(blitzar_core::ParticleStateView particles) noexcept
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

bool BarnesHutSolver::Contains(
    const blitzar_trees::Octree::Cell& cell, blitzar_core::Vector3 position) noexcept
{
    return std::abs(position.x - cell.center.x) <= cell.half_extent &&
           std::abs(position.y - cell.center.y) <= cell.half_extent &&
           std::abs(position.z - cell.center.z) <= cell.half_extent;
}

blitzar_status BarnesHutSolver::Accumulate(const AccumulationRequest& request) noexcept
{
    if (request.stack.empty()) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    std::size_t stack_size = 1;

    request.stack[0] = 0;

    const blitzar_core::Vector3 target_position{
        request.targets.x[request.target], request.targets.y[request.target],
        request.targets.z[request.target]};

    request.acceleration = {};

    while (stack_size > 0) {
        const std::size_t cell_index = request.stack[--stack_size];
        const std::span<const blitzar_trees::Octree::Cell> cell_view =
            request.tree.CellAt(cell_index);

        if (cell_view.empty()) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }

        const blitzar_trees::Octree::Cell& cell = cell_view.front();

        if (cell.mass == 0.0) {
            continue;
        }

        const bool contains_target = Contains(cell, target_position);

        if (cell.IsLeaf()) {
            for (std::size_t offset = 0; offset < cell.count; ++offset) {
                std::size_t source = 0;

                if (!request.tree.ParticleIndex(cell.begin + offset, source)) {
                    return BLITZAR_STATUS_INTERNAL_ERROR;
                }
                if ((request.skip_self && source == request.target) ||
                    request.sources.mass[source] == 0.0) {
                    continue;
                }

                const blitzar_core::Scalar dx = request.sources.x[source] - target_position.x;
                const blitzar_core::Scalar dy = request.sources.y[source] - target_position.y;
                const blitzar_core::Scalar dz = request.sources.z[source] - target_position.z;
                const blitzar_core::Scalar distance_squared = dx * dx + dy * dy + dz * dz;
                const blitzar_physics::PairStatus pair_status =
                    gravity_.ValidatePair(request.sources.mass[source], distance_squared);

                if (pair_status != blitzar_physics::PairStatus::Valid) {
                    return pair_status == blitzar_physics::PairStatus::Singularity
                               ? BLITZAR_STATUS_SINGULARITY
                               : BLITZAR_STATUS_INVALID_ARGUMENT;
                }

                const blitzar_core::Scalar factor =
                    gravity_.PairFactor(request.sources.mass[source], distance_squared);

                if (!std::isfinite(factor)) {
                    return BLITZAR_STATUS_INVALID_ARGUMENT;
                }

                request.acceleration.x += factor * dx;
                request.acceleration.y += factor * dy;
                request.acceleration.z += factor * dz;
            }

            continue;
        }

        const blitzar_core::Scalar dx = cell.center_of_mass.x - target_position.x;
        const blitzar_core::Scalar dy = cell.center_of_mass.y - target_position.y;
        const blitzar_core::Scalar dz = cell.center_of_mass.z - target_position.z;
        const blitzar_core::Scalar distance_squared = dx * dx + dy * dy + dz * dz;
        const blitzar_core::Scalar distance = std::sqrt(distance_squared);
        const blitzar_core::Scalar cell_width = 2.0 * cell.half_extent;

        if (!contains_target && distance > 0.0 && cell_width / distance < settings_.opening_angle) {
            const blitzar_physics::PairStatus pair_status =
                gravity_.ValidatePair(cell.mass, distance_squared);

            if (pair_status != blitzar_physics::PairStatus::Valid) {
                return pair_status == blitzar_physics::PairStatus::Singularity
                           ? BLITZAR_STATUS_SINGULARITY
                           : BLITZAR_STATUS_INVALID_ARGUMENT;
            }

            const blitzar_core::Scalar factor = gravity_.PairFactor(cell.mass, distance_squared);

            if (!std::isfinite(factor)) {
                return BLITZAR_STATUS_INVALID_ARGUMENT;
            }

            request.acceleration.x += factor * dx;
            request.acceleration.y += factor * dy;
            request.acceleration.z += factor * dz;

            continue;
        }

        for (auto child = cell.children.rbegin(); child != cell.children.rend(); ++child) {
            if (*child != blitzar_trees::Octree::Cell::InvalidIndex) {
                if (stack_size == request.stack.size()) {
                    return BLITZAR_STATUS_INTERNAL_ERROR;
                }

                request.stack[stack_size++] = *child;
            }
        }
    }

    if (!std::isfinite(request.acceleration.x) || !std::isfinite(request.acceleration.y) ||
        !std::isfinite(request.acceleration.z)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    return BLITZAR_STATUS_OK;
}

blitzar_status BarnesHutSolver::Compute(blitzar_core::ParticleStateView particles,
    blitzar_core::ForceView forces, const blitzar_core::ExecutionSettings& settings) noexcept
{
    if (!settings_.IsValid()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    return Compute(particles, forces, settings, workspace_);
}

blitzar_status BarnesHutSolver::Compute(blitzar_core::ParticleStateView particles,
    blitzar_core::ForceView forces, const blitzar_core::ExecutionSettings& settings,
    ThreadWorkspace& workspace) noexcept
{
    const blitzar_status prepare_status = Prepare(particles.count);

    if (prepare_status != BLITZAR_STATUS_OK) {
        return prepare_status;
    }

    const blitzar_status status =
        ComputeTree({*tree_, particles, particles, forces, settings, workspace, false, true});

    return status == BLITZAR_STATUS_OK ? CommitStagedForces(forces) : status;
}

blitzar_status BarnesHutSolver::ComputeSplit(const BarnesHutSplitRequest& request) noexcept
{
    return ComputeSplit(request, workspace_);
}

blitzar_status BarnesHutSolver::ComputeSplit(
    const BarnesHutSplitRequest& request, ThreadWorkspace& workspace) noexcept
{
    const blitzar_status prepare_status = Prepare(request.local.count);

    if (prepare_status != BLITZAR_STATUS_OK) {
        return prepare_status;
    }

    if (!settings_.IsValid() || request.remote.SourceCount() == 0) {
        const blitzar_status status = ComputeTree({*tree_, request.local, request.local,
            request.forces, request.settings, workspace, false, true});

        return status == BLITZAR_STATUS_OK ? CommitStagedForces(request.forces) : status;
    }

    if (remote_tree_ == nullptr) {
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
    }

    const blitzar_status local_status = ComputeTree({*tree_, request.local, request.local,
        request.forces, request.settings, workspace, false, true});

    if (local_status != BLITZAR_STATUS_OK) {
        return local_status;
    }

    const blitzar_status remote_status = ComputeTree({*remote_tree_, request.local, request.remote,
        request.forces, request.settings, workspace, true, false});

    return remote_status == BLITZAR_STATUS_OK ? CommitStagedForces(request.forces) : remote_status;
}

blitzar_status BarnesHutSolver::ComputeTree(const TreeComputeRequest& request) noexcept
{
    if (!settings_.IsValid() || !gravity_.IsValid() || !request.settings.IsValid() ||
        request.targets.count != request.forces.count ||
        request.sources.SourceCount() >
            (&request.tree == tree_.get() ? local_particle_capacity_ : settings_.max_particles) ||
        !IsValidState(request.targets) || !IsValidState(request.sources) ||
        !blitzar_core::IsValid(request.forces) ||
        request.workspace.MaxCells() < settings_.max_cells ||
        request.workspace.MaxDepth() < settings_.max_depth ||
        staging_.size() < request.targets.count) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    if (request.sources.SourceCount() == 0) {
        if (!request.accumulate) {
            for (std::size_t index = 0; index < request.targets.count; ++index) {
                staging_[index] = {};
            }
        }

        return BLITZAR_STATUS_OK;
    }
    if (!request.tree.Refit(request.sources)) {
        const blitzar_status build_status = request.tree.Build(request.sources);

        if (build_status != BLITZAR_STATUS_OK) {
            return build_status;
        }
    }

    std::atomic<blitzar_status> computation_status{BLITZAR_STATUS_OK};

#if defined(_OPENMP)
#pragma omp parallel for schedule(static)
#endif

    for (std::int64_t target_index = 0;
         target_index < static_cast<std::int64_t>(request.targets.count); ++target_index) {
        if (computation_status.load(std::memory_order_relaxed) != BLITZAR_STATUS_OK) {
            continue;
        }

        const std::size_t target = static_cast<std::size_t>(target_index);
        blitzar_core::Vector3 acceleration{};
        const AccumulationRequest accumulation{request.tree, request.targets, request.sources,
            target, request.workspace.Stack(ThreadWorkspace::CurrentThread()), acceleration,
            request.skip_self};

        const blitzar_status target_status = Accumulate(accumulation);

        if (target_status != BLITZAR_STATUS_OK) {
            blitzar_status expected = BLITZAR_STATUS_OK;

            computation_status.compare_exchange_strong(
                expected, target_status, std::memory_order_relaxed, std::memory_order_relaxed);

            continue;
        }

        if (request.accumulate) {
            staging_[target].x += acceleration.x;
            staging_[target].y += acceleration.y;
            staging_[target].z += acceleration.z;
        }
        else {
            staging_[target] = acceleration;
        }
    }

    return computation_status.load(std::memory_order_relaxed);
}

blitzar_status BarnesHutSolver::CommitStagedForces(blitzar_core::ForceView forces) noexcept
{
    if (!blitzar_core::IsValid(forces) || forces.count > staging_.size()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    for (std::size_t index = 0; index < forces.count; ++index) {
        forces.x[index] = staging_[index].x;
        forces.y[index] = staging_[index].y;
        forces.z[index] = staging_[index].z;
    }

    return BLITZAR_STATUS_OK;
}

blitzar_status BarnesHutSolver::EnsureLocalCapacity(std::size_t particle_capacity) noexcept
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
        candidate_tree = std::make_unique<blitzar_trees::Octree>(particle_capacity, cell_capacity,
            settings_.leaf_capacity, settings_.max_depth);

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
