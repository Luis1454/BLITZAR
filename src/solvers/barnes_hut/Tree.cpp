#include "solvers/barnes_hut/BarnesHutSolver.hpp"

#include <atomic>
#include <cmath>
#include <cstdint>

namespace blitzar_barnes_hut {

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

blitzar_status BarnesHutSolver::AccumulateSource(const AccumulationRequest& request,
    std::size_t source, blitzar_core::Vector3 target_position) const noexcept
{
    if ((request.skip_self && source == request.target) || request.sources.mass[source] == 0.0) {
        return BLITZAR_STATUS_OK;
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

    return BLITZAR_STATUS_OK;
}

blitzar_status BarnesHutSolver::AccumulateLeaf(const AccumulationRequest& request,
    const blitzar_trees::Octree::Cell& cell, blitzar_core::Vector3 target_position) const noexcept
{
    for (std::size_t offset = 0; offset < cell.count; ++offset) {
        std::size_t source = 0;

        if (!request.tree.ParticleIndex(cell.begin + offset, source)) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }

        const blitzar_status status = AccumulateSource(request, source, target_position);

        if (status != BLITZAR_STATUS_OK) {
            return status;
        }
    }

    return BLITZAR_STATUS_OK;
}

blitzar_status BarnesHutSolver::AccumulateMultipole(const AccumulationRequest& request,
    const blitzar_trees::Octree::Cell& cell, blitzar_core::Vector3 target_position,
    bool& consumed) const noexcept
{
    consumed = false;

    const blitzar_core::Scalar dx = cell.center_of_mass.x - target_position.x;
    const blitzar_core::Scalar dy = cell.center_of_mass.y - target_position.y;
    const blitzar_core::Scalar dz = cell.center_of_mass.z - target_position.z;
    const blitzar_core::Scalar distance_squared = dx * dx + dy * dy + dz * dz;
    const blitzar_core::Scalar distance = std::sqrt(distance_squared);
    const blitzar_core::Scalar cell_width = 2.0 * cell.half_extent;

    if (Contains(cell, target_position) || distance <= 0.0 ||
        cell_width / distance >= settings_.opening_angle) {
        return BLITZAR_STATUS_OK;
    }

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
    consumed = true;

    return BLITZAR_STATUS_OK;
}

blitzar_status BarnesHutSolver::PushChildren(
    const blitzar_trees::Octree::Cell& cell, std::span<std::size_t> stack,
    std::size_t& stack_size) noexcept
{
    for (auto child = cell.children.rbegin(); child != cell.children.rend(); ++child) {
        if (*child == blitzar_trees::Octree::Cell::InvalidIndex) {
            continue;
        }
        if (stack_size == stack.size()) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }

        stack[stack_size++] = *child;
    }

    return BLITZAR_STATUS_OK;
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
        if (cell.IsLeaf()) {
            const blitzar_status status = AccumulateLeaf(request, cell, target_position);

            if (status != BLITZAR_STATUS_OK) {
                return status;
            }

            continue;
        }

        bool consumed = false;
        const blitzar_status multipole_status =
            AccumulateMultipole(request, cell, target_position, consumed);

        if (multipole_status != BLITZAR_STATUS_OK) {
            return multipole_status;
        }
        if (consumed) {
            continue;
        }

        const blitzar_status push_status = PushChildren(cell, request.stack, stack_size);

        if (push_status != BLITZAR_STATUS_OK) {
            return push_status;
        }
    }

    return std::isfinite(request.acceleration.x) && std::isfinite(request.acceleration.y) &&
                   std::isfinite(request.acceleration.z)
               ? BLITZAR_STATUS_OK
               : BLITZAR_STATUS_INVALID_ARGUMENT;
}

bool BarnesHutSolver::ValidateTreeRequest(const TreeComputeRequest& request) const noexcept
{
    const std::size_t source_capacity = &request.tree == tree_.get() ? local_particle_capacity_
                                                                      : settings_.max_particles;

    return settings_.IsValid() && gravity_.IsValid() && request.settings.IsValid() &&
           request.targets.count == request.forces.count &&
           request.sources.SourceCount() <= source_capacity && IsValidState(request.targets) &&
           IsValidState(request.sources) && blitzar_core::IsValid(request.forces) &&
           request.stack_pool.MaxCells() >= settings_.max_cells &&
           request.stack_pool.MaxDepth() >= settings_.max_depth &&
           staging_.size() >= request.targets.count;
}

blitzar_status BarnesHutSolver::PrepareTree(const TreeComputeRequest& request) noexcept
{
    if (request.sources.SourceCount() == 0) {
        if (!request.accumulate) {
            for (std::size_t index = 0; index < request.targets.count; ++index) {
                staging_[index] = {};
            }
        }

        return BLITZAR_STATUS_OK;
    }

    if (request.tree.Refit(request.sources)) {
        return BLITZAR_STATUS_OK;
    }

    return request.tree.Build(request.sources);
}

blitzar_status BarnesHutSolver::ComputeTargets(const TreeComputeRequest& request) noexcept
{
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
            target, request.stack_pool.Stack(ThreadStackPool::CurrentThread()), acceleration,
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

blitzar_status BarnesHutSolver::ComputeTree(const TreeComputeRequest& request) noexcept
{
    if (!ValidateTreeRequest(request)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const blitzar_status preparation_status = PrepareTree(request);

    return preparation_status == BLITZAR_STATUS_OK ? ComputeTargets(request)
                                                   : preparation_status;
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

} // namespace blitzar_barnes_hut
