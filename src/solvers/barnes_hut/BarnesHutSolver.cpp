#include "solvers/barnes_hut/BarnesHutSolver.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <new>
#include <stdexcept>

namespace blitzar_barnes_hut {

bool BarnesHutSettings::IsValid() const noexcept
{
    return std::isfinite(opening_angle) && opening_angle >= 0.0 && max_particles > 0 &&
           max_cells > 0 && leaf_capacity > 0 && max_depth > 0;
}

BarnesHutSolver::BarnesHutSolver(
    blitzar_physics::GravityParameters gravity, BarnesHutSettings settings)
    : settings_(settings), gravity_(gravity),
      tree_(settings.max_particles, settings.max_cells, settings.leaf_capacity, settings.max_depth),
      workspace_(settings.max_cells, settings.max_depth)
{
}

blitzar_core::SolverKind BarnesHutSolver::Kind() const noexcept
{
    return blitzar_core::SolverKind::BarnesHut;
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

blitzar_status BarnesHutSolver::Accumulate(std::size_t target,
    blitzar_core::ParticleStateView particles, std::span<std::size_t> stack,
    blitzar_core::Vector3& acceleration) noexcept
{
    if (stack.empty()) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    std::size_t stack_size = 1;

    stack[0] = 0;

    const blitzar_core::Vector3 target_position{
        particles.x[target], particles.y[target], particles.z[target]};

    acceleration = {};

    while (stack_size > 0) {
        const std::size_t cell_index = stack[--stack_size];
        const std::span<const blitzar_trees::Octree::Cell> cell_view = tree_.CellAt(cell_index);

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

                if (!tree_.ParticleIndex(cell.begin + offset, source)) {
                    return BLITZAR_STATUS_INTERNAL_ERROR;
                }
                if (source == target || particles.mass[source] == 0.0) {
                    continue;
                }

                const blitzar_core::Scalar dx = particles.x[source] - target_position.x;
                const blitzar_core::Scalar dy = particles.y[source] - target_position.y;
                const blitzar_core::Scalar dz = particles.z[source] - target_position.z;
                const blitzar_core::Scalar distance_squared = dx * dx + dy * dy + dz * dz;
                const blitzar_physics::PairStatus pair_status =
                    gravity_.ValidatePair(particles.mass[source], distance_squared);

                if (pair_status != blitzar_physics::PairStatus::Valid) {
                    return pair_status == blitzar_physics::PairStatus::Singularity
                               ? BLITZAR_STATUS_SINGULARITY
                               : BLITZAR_STATUS_INVALID_ARGUMENT;
                }

                const blitzar_core::Scalar factor =
                    gravity_.PairFactor(particles.mass[source], distance_squared);

                if (!std::isfinite(factor)) {
                    return BLITZAR_STATUS_INVALID_ARGUMENT;
                }

                acceleration.x += factor * dx;
                acceleration.y += factor * dy;
                acceleration.z += factor * dz;
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

            acceleration.x += factor * dx;
            acceleration.y += factor * dy;
            acceleration.z += factor * dz;

            continue;
        }

        for (auto child = cell.children.rbegin(); child != cell.children.rend(); ++child) {
            if (*child != blitzar_trees::Octree::Cell::InvalidIndex) {
                if (stack_size == stack.size()) {
                    return BLITZAR_STATUS_INTERNAL_ERROR;
                }

                stack[stack_size++] = *child;
            }
        }
    }

    if (!std::isfinite(acceleration.x) || !std::isfinite(acceleration.y) ||
        !std::isfinite(acceleration.z)) {
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
    if (!settings_.IsValid() || !gravity_.IsValid() || !settings.IsValid() ||
        particles.count != forces.count || particles.SourceCount() > settings_.max_particles ||
        !IsValidState(particles) || !blitzar_core::IsValid(forces) ||
        workspace.MaxCells() < settings_.max_cells || workspace.MaxDepth() < settings_.max_depth) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    if (!tree_.Refit(particles)) {
        const blitzar_status build_status = tree_.Build(particles);

        if (build_status != BLITZAR_STATUS_OK) {
            return build_status;
        }
    }

    std::atomic<blitzar_status> computation_status{BLITZAR_STATUS_OK};

#if defined(_OPENMP)
#pragma omp parallel for schedule(static)
#endif

    for (std::int64_t target_index = 0; target_index < static_cast<std::int64_t>(particles.count);
         ++target_index) {
        const std::size_t target = static_cast<std::size_t>(target_index);

        if (computation_status.load(std::memory_order_relaxed) != BLITZAR_STATUS_OK) {
            continue;
        }

        blitzar_core::Vector3 acceleration{};
        std::span<std::size_t> traversal_stack = workspace.Stack(ThreadWorkspace::CurrentThread());
        const blitzar_status target_status =
            Accumulate(target, particles, traversal_stack, acceleration);

        if (target_status != BLITZAR_STATUS_OK) {
            blitzar_status expected = BLITZAR_STATUS_OK;

            computation_status.compare_exchange_strong(
                expected, target_status, std::memory_order_relaxed, std::memory_order_relaxed);
        }
    }

    if (computation_status.load(std::memory_order_relaxed) != BLITZAR_STATUS_OK) {
        return computation_status.load(std::memory_order_relaxed);
    }

#if defined(_OPENMP)
#pragma omp parallel for schedule(static)
#endif

    for (std::int64_t target_index = 0; target_index < static_cast<std::int64_t>(particles.count);
         ++target_index) {
        const std::size_t target = static_cast<std::size_t>(target_index);

        if (computation_status.load(std::memory_order_relaxed) != BLITZAR_STATUS_OK) {
            continue;
        }

        blitzar_core::Vector3 acceleration{};
        std::span<std::size_t> traversal_stack = workspace.Stack(ThreadWorkspace::CurrentThread());
        const blitzar_status target_status =
            Accumulate(target, particles, traversal_stack, acceleration);

        if (target_status != BLITZAR_STATUS_OK) {
            blitzar_status expected = BLITZAR_STATUS_OK;

            computation_status.compare_exchange_strong(
                expected, target_status, std::memory_order_relaxed, std::memory_order_relaxed);

            continue;
        }

        forces.x[target] = acceleration.x;
        forces.y[target] = acceleration.y;
        forces.z[target] = acceleration.z;
    }

    return computation_status.load(std::memory_order_relaxed);
}

} // namespace blitzar_barnes_hut
