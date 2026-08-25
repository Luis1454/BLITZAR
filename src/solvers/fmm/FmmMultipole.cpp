#include "solvers/fmm/FmmSolver.hpp"

#include <cmath>
#include <limits>

namespace blitzar_fmm {

blitzar_status FmmSolver::PairStatusToStatus(blitzar_physics::PairStatus status) noexcept
{
    if (status == blitzar_physics::PairStatus::Singularity) {
        return BLITZAR_STATUS_SINGULARITY;
    }
    if (status == blitzar_physics::PairStatus::Invalid) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    return BLITZAR_STATUS_OK;
}

bool FmmSolver::IsValidState(blitzar_core::ParticleStateView particles) noexcept
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

bool FmmSolver::Contains(
    const blitzar_trees::Octree::Cell& cell, blitzar_core::Vector3 position) noexcept
{
    return std::abs(position.x - cell.center.x) <= cell.half_extent &&
           std::abs(position.y - cell.center.y) <= cell.half_extent &&
           std::abs(position.z - cell.center.z) <= cell.half_extent;
}

bool FmmSolver::IsFiniteMultipole(const Multipole& multipole) noexcept
{
    return std::isfinite(multipole.mass) && std::isfinite(multipole.xx) &&
           std::isfinite(multipole.xy) && std::isfinite(multipole.xz) &&
           std::isfinite(multipole.yy) && std::isfinite(multipole.yz) &&
           std::isfinite(multipole.zz);
}

blitzar_status FmmSolver::BuildLeafMultipole(const blitzar_trees::Octree& tree,
    const blitzar_trees::Octree::Cell& cell, blitzar_core::ParticleStateView sources,
    Multipole& multipole) const noexcept
{
    for (std::size_t offset = 0; offset < cell.count; ++offset) {
        if (cell.begin > std::numeric_limits<std::size_t>::max() - offset) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }

        std::size_t particle = 0;

        if (!tree.ParticleIndex(cell.begin + offset, particle) ||
            particle >= sources.SourceCount()) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }

        const blitzar_core::Scalar mass = sources.mass[particle];
        const blitzar_core::Scalar dx = sources.x[particle] - cell.center_of_mass.x;
        const blitzar_core::Scalar dy = sources.y[particle] - cell.center_of_mass.y;
        const blitzar_core::Scalar dz = sources.z[particle] - cell.center_of_mass.z;

        multipole.xx += mass * dx * dx;
        multipole.xy += mass * dx * dy;
        multipole.xz += mass * dx * dz;
        multipole.yy += mass * dy * dy;
        multipole.yz += mass * dy * dz;
        multipole.zz += mass * dz * dz;
    }

    return IsFiniteMultipole(multipole) ? BLITZAR_STATUS_OK
                                       : BLITZAR_STATUS_INVALID_ARGUMENT;
}

blitzar_status FmmSolver::MergeChildMultipoles(
    std::span<const blitzar_trees::Octree::Cell> cells,
    const blitzar_trees::Octree::Cell& cell, std::span<const Multipole> multipoles,
    Multipole& result) const noexcept
{
    for (const std::size_t child : cell.children) {
        if (child == blitzar_trees::Octree::Cell::InvalidIndex) {
            continue;
        }
        if (child >= cells.size() || child >= multipoles.size()) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }

        const blitzar_trees::Octree::Cell& child_cell = cells[child];

        const Multipole& child_multipole = multipoles[child];

        const blitzar_core::Scalar dx =
            child_cell.center_of_mass.x - cell.center_of_mass.x;

        const blitzar_core::Scalar dy =
            child_cell.center_of_mass.y - cell.center_of_mass.y;

        const blitzar_core::Scalar dz =
            child_cell.center_of_mass.z - cell.center_of_mass.z;

        result.xx += child_multipole.xx + child_multipole.mass * dx * dx;
        result.xy += child_multipole.xy + child_multipole.mass * dx * dy;
        result.xz += child_multipole.xz + child_multipole.mass * dx * dz;
        result.yy += child_multipole.yy + child_multipole.mass * dy * dy;
        result.yz += child_multipole.yz + child_multipole.mass * dy * dz;
        result.zz += child_multipole.zz + child_multipole.mass * dz * dz;
    }

    return IsFiniteMultipole(result) ? BLITZAR_STATUS_OK
                                     : BLITZAR_STATUS_INVALID_ARGUMENT;
}

blitzar_status FmmSolver::BuildMultipoles(const blitzar_trees::Octree& tree,
    blitzar_core::ParticleStateView sources, std::vector<Multipole>& multipoles) const noexcept
{
    const std::span<const blitzar_trees::Octree::Cell> cells = tree.Cells();

    if (cells.empty() || cells.size() > multipoles.capacity()) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    multipoles.resize(cells.size());

    for (std::size_t index = cells.size(); index-- > 0;) {
        const blitzar_trees::Octree::Cell& cell = cells[index];

        Multipole& multipole = multipoles[index];
        multipole = {};
        multipole.mass = cell.mass;

        const blitzar_status status = cell.IsLeaf()
                                          ? BuildLeafMultipole(tree, cell, sources, multipole)
                                          : MergeChildMultipoles(cells, cell, multipoles, multipole);

        if (status != BLITZAR_STATUS_OK) {
            return status;
        }
    }

    return BLITZAR_STATUS_OK;
}

blitzar_status FmmSolver::EvaluateMultipole(const Multipole& multipole,
    blitzar_core::Vector3 displacement, blitzar_core::Scalar squared_distance,
    blitzar_core::Vector3& acceleration) const noexcept
{
    if (multipole.mass == 0.0) {
        return BLITZAR_STATUS_OK;
    }

    const blitzar_status pair_status = PairStatusToStatus(
        gravity_.ValidatePair(multipole.mass, squared_distance));

    if (pair_status != BLITZAR_STATUS_OK) {
        return pair_status;
    }

    const blitzar_core::Scalar softening = parameters_.EffectiveSoftening();
    const blitzar_core::Scalar softened_squared = squared_distance + softening * softening;
    const blitzar_core::Scalar inverse_squared = 1.0 / softened_squared;
    const blitzar_core::Scalar inverse_distance = 1.0 / std::sqrt(softened_squared);
    const blitzar_core::Scalar inverse_cubed = inverse_squared * inverse_distance;
    const blitzar_core::Scalar inverse_fifth = inverse_cubed * inverse_squared;
    const blitzar_core::Scalar inverse_seventh = inverse_fifth * inverse_squared;
    const blitzar_core::Vector3 moment_times_displacement{
        multipole.xx * displacement.x + multipole.xy * displacement.y +
            multipole.xz * displacement.z,
        multipole.xy * displacement.x + multipole.yy * displacement.y +
            multipole.yz * displacement.z,
        multipole.xz * displacement.x + multipole.yz * displacement.y +
            multipole.zz * displacement.z};

    const blitzar_core::Scalar trace = multipole.xx + multipole.yy + multipole.zz;
    const blitzar_core::Scalar contraction =
        displacement.x * moment_times_displacement.x +
        displacement.y * moment_times_displacement.y +
        displacement.z * moment_times_displacement.z;

    const blitzar_core::Scalar constant = parameters_.EffectiveConstant();

    acceleration.x += constant *
        (multipole.mass * displacement.x * inverse_cubed -
            3.0 * moment_times_displacement.x * inverse_fifth -
            1.5 * displacement.x * trace * inverse_fifth +
            7.5 * displacement.x * contraction * inverse_seventh);

    acceleration.y += constant *
        (multipole.mass * displacement.y * inverse_cubed -
            3.0 * moment_times_displacement.y * inverse_fifth -
            1.5 * displacement.y * trace * inverse_fifth +
            7.5 * displacement.y * contraction * inverse_seventh);

    acceleration.z += constant *
        (multipole.mass * displacement.z * inverse_cubed -
            3.0 * moment_times_displacement.z * inverse_fifth -
            1.5 * displacement.z * trace * inverse_fifth +
            7.5 * displacement.z * contraction * inverse_seventh);

    return std::isfinite(acceleration.x) && std::isfinite(acceleration.y) &&
                   std::isfinite(acceleration.z)
               ? BLITZAR_STATUS_OK
               : BLITZAR_STATUS_INVALID_ARGUMENT;
}

} // namespace blitzar_fmm
