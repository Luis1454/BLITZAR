#include "trees/Octree.hpp"

#include <cmath>
#include <limits>

namespace blitzar_trees {

blitzar_status Octree::CalculateLeafProperties(
    Cell& cell, blitzar_core::ParticleStateView particles) noexcept
{
    cell.mass = 0.0;
    cell.center_of_mass = {};

    for (std::size_t offset = 0; offset < cell.count; ++offset) {
        if (offset > particle_count_ || cell.begin > particle_count_ - offset ||
            cell.begin + offset >= indices_.size()) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }

        const std::size_t particle = indices_[cell.begin + offset];

        if (particle >= particles.SourceCount()) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }

        const blitzar_core::Scalar mass = particles.mass[particle];

        cell.mass += mass;
        cell.center_of_mass.x += mass * particles.x[particle];
        cell.center_of_mass.y += mass * particles.y[particle];
        cell.center_of_mass.z += mass * particles.z[particle];
    }

    return BLITZAR_STATUS_OK;
}

blitzar_status Octree::CalculateInternalProperties(Cell& cell) noexcept
{
    cell.mass = 0.0;
    cell.center_of_mass = {};

    for (const std::size_t child : cell.children) {
        if (child == Cell::InvalidIndex) {
            continue;
        }
        if (child >= cells_.size()) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }

        const Cell& child_cell = cells_[child];
        cell.mass += child_cell.mass;
        cell.center_of_mass.x += child_cell.mass * child_cell.center_of_mass.x;
        cell.center_of_mass.y += child_cell.mass * child_cell.center_of_mass.y;
        cell.center_of_mass.z += child_cell.mass * child_cell.center_of_mass.z;
    }

    return BLITZAR_STATUS_OK;
}

blitzar_status Octree::FinalizeProperties(Cell& cell) noexcept
{
    if (!std::isfinite(cell.mass)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    if (cell.mass > 0.0) {
        cell.center_of_mass.x /= cell.mass;
        cell.center_of_mass.y /= cell.mass;
        cell.center_of_mass.z /= cell.mass;
    }
    else {
        cell.center_of_mass = cell.center;
    }

    return std::isfinite(cell.center_of_mass.x) && std::isfinite(cell.center_of_mass.y) &&
                   std::isfinite(cell.center_of_mass.z)
               ? BLITZAR_STATUS_OK
               : BLITZAR_STATUS_INVALID_ARGUMENT;
}

blitzar_status Octree::CalculateProperties(blitzar_core::ParticleStateView particles) noexcept
{
    for (std::size_t index = cells_.size(); index-- > 0;) {
        Cell& cell = cells_[index];

        const blitzar_status accumulation_status = cell.IsLeaf()
                                                       ? CalculateLeafProperties(cell, particles)
                                                       : CalculateInternalProperties(cell);

        if (accumulation_status != BLITZAR_STATUS_OK) {
            return accumulation_status;
        }

        const blitzar_status final_status = FinalizeProperties(cell);

        if (final_status != BLITZAR_STATUS_OK) {
            return final_status;
        }
    }

    return BLITZAR_STATUS_OK;
}

bool Octree::Refit(blitzar_core::ParticleStateView particles) noexcept
{
    if (particle_count_ == 0 || particles.SourceCount() != particle_count_ ||
        !IsValidInput(particles)) {
        return false;
    }

    for (const Cell& cell : cells_) {
        if (!cell.IsLeaf()) {
            continue;
        }

        for (std::size_t offset = 0; offset < cell.count; ++offset) {
            if (offset > particle_count_ || cell.begin > particle_count_ - offset ||
                cell.begin + offset >= indices_.size()) {
                return false;
            }

            const std::size_t particle = indices_[cell.begin + offset];

            if (particle >= particles.SourceCount()) {
                return false;
            }

            const blitzar_core::Vector3 position{
                particles.x[particle], particles.y[particle], particles.z[particle]};

            if (!Contains(cell, position)) {
                return false;
            }
        }
    }

    if (CalculateProperties(particles) != BLITZAR_STATUS_OK) {
        return false;
    }

    ++refit_count_;

    return true;
}

} // namespace blitzar_trees
