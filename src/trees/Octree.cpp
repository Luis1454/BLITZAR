#include "trees/Octree.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace blitzar_trees {

bool Octree::Cell::IsLeaf() const noexcept
{
    return children[0] < 0;
}

Octree::Octree(
    std::size_t max_particles,
    std::size_t max_cells,
    std::size_t leaf_capacity,
    std::size_t max_depth)
    : max_particles_(max_particles),
      max_cells_(max_cells),
      leaf_capacity_(leaf_capacity),
      max_depth_(max_depth),
      particle_count_(0),
      indices_(max_particles),
      scratch_(max_particles)
{
    cells_.reserve(max_cells);
}

bool Octree::IsValidInput(blitzar_core::ParticleStateView particles) noexcept
{
    if (!blitzar_core::IsValid(particles)) {
        return false;
    }
    for (std::size_t index = 0; index < particles.count; ++index) {
        if (!std::isfinite(particles.x[index]) ||
            !std::isfinite(particles.y[index]) ||
            !std::isfinite(particles.z[index]) ||
            !std::isfinite(particles.mass[index]) || particles.mass[index] < 0.0) {
            return false;
        }
    }
    return true;
}

Octree::Cell Octree::MakeCell(
    blitzar_core::Vector3 center,
    blitzar_core::Scalar half_extent,
    std::size_t begin,
    std::size_t count,
    std::size_t depth) const noexcept
{
    Cell cell{};
    cell.center = center;
    cell.half_extent = half_extent;
    cell.begin = begin;
    cell.count = count;
    cell.depth = depth;
    cell.children.fill(-1);
    return cell;
}

int Octree::Octant(
    const Cell& cell, blitzar_core::Vector3 position) noexcept
{
    return (position.x >= cell.center.x ? 1 : 0) |
           (position.y >= cell.center.y ? 2 : 0) |
           (position.z >= cell.center.z ? 4 : 0);
}

void Octree::Partition(
    const Cell& cell,
    blitzar_core::ParticleStateView particles,
    std::array<std::size_t, 8>& counts) noexcept
{
    counts.fill(0);
    for (std::size_t offset = 0; offset < cell.count; ++offset) {
        const std::size_t particle = indices_[cell.begin + offset];
        ++counts[Octant(cell, {particles.x[particle], particles.y[particle],
                               particles.z[particle]})];
    }

    std::array<std::size_t, 8> write{};
    write[0] = cell.begin;
    for (std::size_t octant = 1; octant < write.size(); ++octant) {
        write[octant] = write[octant - 1] + counts[octant - 1];
    }
    for (std::size_t offset = 0; offset < cell.count; ++offset) {
        const std::size_t particle = indices_[cell.begin + offset];
        const int octant = Octant(
            cell, {particles.x[particle], particles.y[particle], particles.z[particle]});
        scratch_[write[octant]++] = particle;
    }
    for (std::size_t offset = 0; offset < cell.count; ++offset) {
        indices_[cell.begin + offset] = scratch_[cell.begin + offset];
    }
}

void Octree::CalculateProperties(
    blitzar_core::ParticleStateView particles) noexcept
{
    for (std::size_t index = cells_.size(); index-- > 0;) {
        Cell& cell = cells_[index];
        cell.mass = 0.0;
        cell.center_of_mass = {};
        if (cell.IsLeaf()) {
            for (std::size_t offset = 0; offset < cell.count; ++offset) {
                const std::size_t particle = indices_[cell.begin + offset];
                const blitzar_core::Scalar mass = particles.mass[particle];
                cell.mass += mass;
                cell.center_of_mass.x += mass * particles.x[particle];
                cell.center_of_mass.y += mass * particles.y[particle];
                cell.center_of_mass.z += mass * particles.z[particle];
            }
        } else {
            for (const int child : cell.children) {
                if (child < 0) {
                    continue;
                }
                const Cell& child_cell = cells_[static_cast<std::size_t>(child)];
                cell.mass += child_cell.mass;
                cell.center_of_mass.x +=
                    child_cell.mass * child_cell.center_of_mass.x;
                cell.center_of_mass.y +=
                    child_cell.mass * child_cell.center_of_mass.y;
                cell.center_of_mass.z +=
                    child_cell.mass * child_cell.center_of_mass.z;
            }
        }
        if (cell.mass > 0.0) {
            cell.center_of_mass.x /= cell.mass;
            cell.center_of_mass.y /= cell.mass;
            cell.center_of_mass.z /= cell.mass;
        } else {
            cell.center_of_mass = cell.center;
        }
    }
}

blitzar_status Octree::Build(blitzar_core::ParticleStateView particles) noexcept
{
    cells_.clear();
    particle_count_ = 0;
    if (particles.count > max_particles_ || leaf_capacity_ == 0 ||
        max_cells_ == 0 || !IsValidInput(particles)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    if (particles.count == 0) {
        return BLITZAR_STATUS_OK;
    }
    particle_count_ = particles.count;

    blitzar_core::Vector3 minimum{
        particles.x[0], particles.y[0], particles.z[0]};
    blitzar_core::Vector3 maximum = minimum;
    for (std::size_t index = 0; index < particles.count; ++index) {
        minimum.x = std::min(minimum.x, particles.x[index]);
        minimum.y = std::min(minimum.y, particles.y[index]);
        minimum.z = std::min(minimum.z, particles.z[index]);
        maximum.x = std::max(maximum.x, particles.x[index]);
        maximum.y = std::max(maximum.y, particles.y[index]);
        maximum.z = std::max(maximum.z, particles.z[index]);
        indices_[index] = index;
    }
    const blitzar_core::Scalar span = std::max(
        {maximum.x - minimum.x, maximum.y - minimum.y, maximum.z - minimum.z});
    const blitzar_core::Scalar half_extent =
        std::max(0.5 * span, std::numeric_limits<blitzar_core::Scalar>::epsilon());
    const blitzar_core::Vector3 center{
        0.5 * (minimum.x + maximum.x),
        0.5 * (minimum.y + maximum.y),
        0.5 * (minimum.z + maximum.z)};
    cells_.push_back(MakeCell(center, half_extent, 0, particles.count, 0));

    for (std::size_t index = 0; index < cells_.size(); ++index) {
        const Cell cell = cells_[index];
        if (cell.count <= leaf_capacity_ || cell.depth >= max_depth_) {
            continue;
        }
        if (cells_.size() + 8 > max_cells_) {
            cells_.clear();
            particle_count_ = 0;
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
        std::array<std::size_t, 8> counts{};
        Partition(cell, particles, counts);
        const blitzar_core::Scalar child_half_extent = 0.5 * cell.half_extent;
        for (std::size_t octant = 0; octant < counts.size(); ++octant) {
            if (counts[octant] == 0) {
                continue;
            }
            const blitzar_core::Vector3 child_center{
                cell.center.x + ((octant & 1U) != 0U ? child_half_extent : -child_half_extent),
                cell.center.y + ((octant & 2U) != 0U ? child_half_extent : -child_half_extent),
                cell.center.z + ((octant & 4U) != 0U ? child_half_extent : -child_half_extent)};
            std::size_t begin = cell.begin;
            for (std::size_t previous = 0; previous < octant; ++previous) {
                begin += counts[previous];
            }
            const int child = static_cast<int>(cells_.size());
            cells_.push_back(MakeCell(
                child_center,
                child_half_extent,
                begin,
                counts[octant],
                cell.depth + 1));
            cells_[index].children[octant] = child;
        }
    }
    CalculateProperties(particles);
    return BLITZAR_STATUS_OK;
}

std::size_t Octree::CellCount() const noexcept
{
    return cells_.size();
}

std::size_t Octree::ParticleCount() const noexcept
{
    return particle_count_;
}

const Octree::Cell& Octree::CellAt(std::size_t index) const noexcept
{
    return cells_[index];
}

std::size_t Octree::ParticleIndex(std::size_t index) const noexcept
{
    return indices_[index];
}

}  // namespace blitzar_trees
