#include "solvers/fmm/FmmSolver.hpp"

#include <atomic>
#include <cmath>
#include <cstdint>
#include <limits>

namespace blitzar_fmm {

blitzar_status FmmSolver::AccumulateLeaf(const AccumulationRequest& request,
    const blitzar_trees::Octree::Cell& cell, blitzar_core::Vector3 target_position) const noexcept
{
    for (std::size_t offset = 0; offset < cell.count; ++offset) {
        if (cell.begin > std::numeric_limits<std::size_t>::max() - offset) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }

        std::size_t source = 0;

        if (!request.tree.ParticleIndex(cell.begin + offset, source) ||
            source >= request.sources.SourceCount()) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }
        if ((request.skip_self && source == request.target) ||
            request.sources.mass[source] == 0.0) {
            continue;
        }

        const blitzar_core::Scalar dx = request.sources.x[source] - target_position.x;
        const blitzar_core::Scalar dy = request.sources.y[source] - target_position.y;
        const blitzar_core::Scalar dz = request.sources.z[source] - target_position.z;
        const blitzar_core::Scalar squared_distance = dx * dx + dy * dy + dz * dz;
        const blitzar_status pair_status = FmmSolver::PairStatusToStatus(
            gravity_.ValidatePair(request.sources.mass[source], squared_distance));

        if (pair_status != BLITZAR_STATUS_OK) {
            return pair_status;
        }

        const blitzar_core::Scalar factor =
            gravity_.PairFactor(request.sources.mass[source], squared_distance);

        if (!std::isfinite(factor)) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        request.acceleration.x += factor * dx;
        request.acceleration.y += factor * dy;
        request.acceleration.z += factor * dz;
    }

    return BLITZAR_STATUS_OK;
}

blitzar_status FmmSolver::PushChildren(const AccumulationRequest& request,
    const blitzar_trees::Octree::Cell& cell, std::size_t& stack_size) noexcept
{
    for (auto child = cell.children.rbegin(); child != cell.children.rend(); ++child) {
        if (*child == blitzar_trees::Octree::Cell::InvalidIndex) {
            continue;
        }
        if (*child >= request.multipoles.size() || stack_size == request.stack.size()) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }

        request.stack[stack_size++] = *child;
    }

    return BLITZAR_STATUS_OK;
}

blitzar_status FmmSolver::ProcessCell(const AccumulationRequest& request, std::size_t cell_index,
    blitzar_core::Vector3 target_position, std::size_t& stack_size) const noexcept
{
    const std::span<const blitzar_trees::Octree::Cell> cell_view = request.tree.CellAt(cell_index);

    if (cell_view.empty() || cell_index >= request.multipoles.size()) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    const blitzar_trees::Octree::Cell& cell = cell_view.front();

    const Multipole& multipole = request.multipoles[cell_index];

    if (multipole.mass == 0.0) {
        return BLITZAR_STATUS_OK;
    }
    if (cell.IsLeaf()) {
        return AccumulateLeaf(request, cell, target_position);
    }

    const blitzar_core::Scalar dx = cell.center_of_mass.x - target_position.x;
    const blitzar_core::Scalar dy = cell.center_of_mass.y - target_position.y;
    const blitzar_core::Scalar dz = cell.center_of_mass.z - target_position.z;
    const blitzar_core::Scalar squared_distance = dx * dx + dy * dy + dz * dz;
    const blitzar_core::Scalar distance = std::sqrt(squared_distance);

    if (!Contains(cell, target_position) && distance > 0.0 &&
        2.0 * cell.half_extent / distance < settings_.opening_angle) {
        return EvaluateMultipole(multipole, {dx, dy, dz}, squared_distance, request.acceleration);
    }

    return PushChildren(request, cell, stack_size);
}

blitzar_status FmmSolver::Accumulate(const AccumulationRequest& request) const noexcept
{
    if (request.stack.empty()) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    std::size_t stack_size = 1;

    request.stack[0] = 0;
    request.acceleration = {};

    const blitzar_core::Vector3 target_position{request.targets.x[request.target],
        request.targets.y[request.target], request.targets.z[request.target]};

    while (stack_size > 0) {
        const blitzar_status status =
            ProcessCell(request, request.stack[--stack_size], target_position, stack_size);

        if (status != BLITZAR_STATUS_OK) {
            return status;
        }
    }

    return std::isfinite(request.acceleration.x) && std::isfinite(request.acceleration.y) &&
                   std::isfinite(request.acceleration.z)
               ? BLITZAR_STATUS_OK
               : BLITZAR_STATUS_INVALID_ARGUMENT;
}

bool FmmSolver::IsKnownTree(const TreeComputeRequest& request) const noexcept
{
    return (tree_ != nullptr && &request.tree == tree_.get()) ||
           (remote_tree_ != nullptr && &request.tree == remote_tree_.get());
}

bool FmmSolver::HasValidTreeState(
    const TreeComputeRequest& request, std::size_t source_capacity) const noexcept
{
    return request.targets.count == request.forces.count &&
           request.sources.SourceCount() <= source_capacity && IsValidState(request.targets) &&
           IsValidState(request.sources) && blitzar_core::IsValid(request.forces);
}

bool FmmSolver::HasValidTreeResources(const TreeComputeRequest& request) const noexcept
{
    return settings_.IsValid() && gravity_.IsValid() && request.settings.IsValid() &&
           request.stack_pool.MaxCells() >= settings_.max_cells &&
           request.stack_pool.MaxDepth() >= settings_.max_depth &&
           staging_.size() >= request.targets.count;
}

bool FmmSolver::ValidateTreeRequest(const TreeComputeRequest& request) const noexcept
{
    const bool local_tree = tree_ != nullptr && &request.tree == tree_.get();
    const std::size_t source_capacity =
        local_tree ? local_particle_capacity_ : settings_.max_particles;

    return IsKnownTree(request) && HasValidTreeResources(request) &&
           HasValidTreeState(request, source_capacity);
}

blitzar_status FmmSolver::PrepareTree(const TreeComputeRequest& request) noexcept
{
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

    return BuildMultipoles(request.tree, request.sources, request.multipoles);
}

blitzar_status FmmSolver::ComputeTargets(const TreeComputeRequest& request) noexcept
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
        const AccumulationRequest accumulation{request.tree, request.multipoles, request.targets,
            request.sources, target,
            request.stack_pool.Stack(blitzar_solver_threading::ThreadStackPool::CurrentThread()),
            acceleration, request.skip_self};

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

blitzar_status FmmSolver::ComputeTree(const TreeComputeRequest& request) noexcept
{
    if (!ValidateTreeRequest(request)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const blitzar_status preparation_status = PrepareTree(request);

    return preparation_status == BLITZAR_STATUS_OK ? ComputeTargets(request) : preparation_status;
}

blitzar_status FmmSolver::CommitStagedForces(blitzar_core::ForceView forces) noexcept
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

} // namespace blitzar_fmm
