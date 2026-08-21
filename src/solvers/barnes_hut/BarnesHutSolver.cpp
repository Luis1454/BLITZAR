#include "solvers/barnes_hut/BarnesHutSolver.hpp"

#include <algorithm>
#include <cmath>

namespace blitzar_barnes_hut {

bool BarnesHutSettings::IsValid() const noexcept
{
    return std::isfinite(opening_angle) && opening_angle >= 0.0 &&
           max_particles > 0 && max_cells > 0 && leaf_capacity > 0 &&
           max_depth > 0;
}

BarnesHutSolver::BarnesHutSolver(
    blitzar_physics::GravityParameters gravity,
    BarnesHutSettings settings)
    : settings_(settings),
      gravity_(gravity),
      tree_(
          settings.max_particles,
          settings.max_cells,
          settings.leaf_capacity,
          settings.max_depth)
{
    stack_.reserve(settings.max_cells);
}

blitzar_core::SolverKind BarnesHutSolver::Kind() const noexcept
{
    return blitzar_core::SolverKind::BarnesHut;
}

bool BarnesHutSolver::IsValidState(
    blitzar_core::ParticleStateView particles) noexcept
{
    if (!blitzar_core::IsValid(particles)) {
        return false;
    }
    for (std::size_t index = 0; index < particles.count; ++index) {
        if (!std::isfinite(particles.x[index]) ||
            !std::isfinite(particles.y[index]) ||
            !std::isfinite(particles.z[index]) ||
            !std::isfinite(particles.velocity_x[index]) ||
            !std::isfinite(particles.velocity_y[index]) ||
            !std::isfinite(particles.velocity_z[index]) ||
            !std::isfinite(particles.mass[index]) || particles.mass[index] < 0.0) {
            return false;
        }
    }
    return true;
}

bool BarnesHutSolver::Contains(
    const blitzar_trees::Octree::Cell& cell,
    blitzar_core::Vector3 position) noexcept
{
    return std::abs(position.x - cell.center.x) <= cell.half_extent &&
           std::abs(position.y - cell.center.y) <= cell.half_extent &&
           std::abs(position.z - cell.center.z) <= cell.half_extent;
}

blitzar_status BarnesHutSolver::Accumulate(
    std::size_t target,
    blitzar_core::ParticleStateView particles,
    blitzar_core::ForceView forces) noexcept
{
    stack_.clear();
    stack_.push_back(0);
    const blitzar_core::Vector3 target_position{
        particles.x[target], particles.y[target], particles.z[target]};
    blitzar_core::Scalar acceleration_x = 0.0;
    blitzar_core::Scalar acceleration_y = 0.0;
    blitzar_core::Scalar acceleration_z = 0.0;

    while (!stack_.empty()) {
        const int cell_index = stack_.back();
        stack_.pop_back();
        const blitzar_trees::Octree::Cell& cell =
            tree_.CellAt(static_cast<std::size_t>(cell_index));
        if (cell.mass == 0.0) {
            continue;
        }
        const bool contains_target = Contains(cell, target_position);
        if (cell.IsLeaf()) {
            for (std::size_t offset = 0; offset < cell.count; ++offset) {
                const std::size_t source = tree_.ParticleIndex(cell.begin + offset);
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
                acceleration_x += factor * dx;
                acceleration_y += factor * dy;
                acceleration_z += factor * dz;
            }
            continue;
        }

        const blitzar_core::Scalar dx = cell.center_of_mass.x - target_position.x;
        const blitzar_core::Scalar dy = cell.center_of_mass.y - target_position.y;
        const blitzar_core::Scalar dz = cell.center_of_mass.z - target_position.z;
        const blitzar_core::Scalar distance_squared = dx * dx + dy * dy + dz * dz;
        const blitzar_core::Scalar distance = std::sqrt(distance_squared);
        const blitzar_core::Scalar cell_width = 2.0 * cell.half_extent;
        if (!contains_target && distance > 0.0 &&
            cell_width / distance < settings_.opening_angle) {
            const blitzar_physics::PairStatus pair_status =
                gravity_.ValidatePair(cell.mass, distance_squared);
            if (pair_status != blitzar_physics::PairStatus::Valid) {
                return pair_status == blitzar_physics::PairStatus::Singularity
                           ? BLITZAR_STATUS_SINGULARITY
                           : BLITZAR_STATUS_INVALID_ARGUMENT;
            }
            const blitzar_core::Scalar factor =
                gravity_.PairFactor(cell.mass, distance_squared);
            if (!std::isfinite(factor)) {
                return BLITZAR_STATUS_INVALID_ARGUMENT;
            }
            acceleration_x += factor * dx;
            acceleration_y += factor * dy;
            acceleration_z += factor * dz;
            continue;
        }
        for (auto child = cell.children.rbegin(); child != cell.children.rend(); ++child) {
            if (*child >= 0) {
                stack_.push_back(*child);
            }
        }
    }
    forces.x[target] = acceleration_x;
    forces.y[target] = acceleration_y;
    forces.z[target] = acceleration_z;
    return BLITZAR_STATUS_OK;
}

blitzar_status BarnesHutSolver::Compute(
    blitzar_core::ParticleStateView particles,
    blitzar_core::ForceView forces,
    const blitzar_core::ExecutionSettings& settings) noexcept
{
    if (!settings_.IsValid() || !gravity_.IsValid() ||
        !settings.IsValid() || particles.count != forces.count ||
        particles.count > settings_.max_particles || !IsValidState(particles) ||
        !blitzar_core::IsValid(forces)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    if (!tree_.Refit(particles)) {
        const blitzar_status build_status = tree_.Build(particles);
        if (build_status != BLITZAR_STATUS_OK) {
            return build_status;
        }
    }
    for (std::size_t target = 0; target < particles.count; ++target) {
        const blitzar_status status = Accumulate(target, particles, forces);
        if (status != BLITZAR_STATUS_OK) {
            return status;
        }
    }
    return BLITZAR_STATUS_OK;
}

}  // namespace blitzar_barnes_hut
