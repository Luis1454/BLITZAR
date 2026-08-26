#include "trees/octree/Octree.hpp"

#include <algorithm>
#include <limits>

namespace blitzar_trees {

blitzar_status Octree::Partition(const Cell& cell, blitzar_core::ParticleStateView particles,
    std::array<std::size_t, 8>& counts) noexcept
{
    if (cell.begin > particle_count_ || cell.count > particle_count_ - cell.begin) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    counts.fill(0);

    for (std::size_t offset = 0; offset < cell.count; ++offset) {
        const std::size_t particle = indices_[cell.begin + offset];

        if (particle >= particles.SourceCount()) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }

        ++counts[Octant(
            cell, {particles.x[particle], particles.y[particle], particles.z[particle]})];
    }

    std::array<std::size_t, 8> write{};

    write[0] = cell.begin;

    for (std::size_t octant = 1; octant < write.size(); ++octant) {
        write[octant] = write[octant - 1] + counts[octant - 1];
    }
    for (std::size_t offset = 0; offset < cell.count; ++offset) {
        const std::size_t particle = indices_[cell.begin + offset];
        const std::size_t octant =
            Octant(cell, {particles.x[particle], particles.y[particle], particles.z[particle]});

        scratch_[write[octant]++] = particle;
    }
    for (std::size_t offset = 0; offset < cell.count; ++offset) {
        indices_[cell.begin + offset] = scratch_[cell.begin + offset];
    }

    return BLITZAR_STATUS_OK;
}

blitzar_status Octree::AppendChildren(
    std::size_t parent_index, const Cell& cell, const std::array<std::size_t, 8>& counts) noexcept
{
    std::size_t child_count = 0;

    for (const std::size_t count : counts) {
        if (count > 0) {
            ++child_count;
        }
    }

    if (cells_.size() > max_cells_ || child_count > max_cells_ - cells_.size()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

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

        const std::size_t child = cells_.size();

        cells_.push_back(
            MakeCell({child_center, child_half_extent, begin, counts[octant], cell.depth + 1}));

        cells_[parent_index].children[octant] = child;
    }

    return BLITZAR_STATUS_OK;
}

blitzar_status Octree::ExpandCell(std::size_t parent_index, const Cell& cell,
    blitzar_core::ParticleStateView particles, std::array<std::size_t, 8>& counts) noexcept
{
    const blitzar_status partition_status = Partition(cell, particles, counts);

    return partition_status == BLITZAR_STATUS_OK ? AppendChildren(parent_index, cell, counts)
                                                 : partition_status;
}

blitzar_status Octree::PrepareBuild(blitzar_core::ParticleStateView particles,
    blitzar_core::Vector3& minimum, blitzar_core::Vector3& maximum) noexcept
{
    if (particles.SourceCount() > max_particles_ || leaf_capacity_ == 0 || max_cells_ == 0 ||
        !IsValidInput(particles)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    if (particles.SourceCount() == 0) {
        return BLITZAR_STATUS_OK;
    }

    particle_count_ = particles.SourceCount();
    minimum = {particles.x[0], particles.y[0], particles.z[0]};
    maximum = minimum;

    for (std::size_t index = 0; index < particle_count_; ++index) {
        minimum.x = std::min(minimum.x, particles.x[index]);
        minimum.y = std::min(minimum.y, particles.y[index]);
        minimum.z = std::min(minimum.z, particles.z[index]);
        maximum.x = std::max(maximum.x, particles.x[index]);
        maximum.y = std::max(maximum.y, particles.y[index]);
        maximum.z = std::max(maximum.z, particles.z[index]);
        indices_[index] = index;
    }

    ParallelMortonSort(particles, minimum, maximum);

    return BLITZAR_STATUS_OK;
}

blitzar_status Octree::BuildCells(blitzar_core::ParticleStateView particles,
    blitzar_core::Vector3 minimum, blitzar_core::Vector3 maximum) noexcept
{
    const blitzar_core::Scalar span =
        std::max({maximum.x - minimum.x, maximum.y - minimum.y, maximum.z - minimum.z});

    const blitzar_core::Scalar half_extent =
        std::max(0.5 * span, std::numeric_limits<blitzar_core::Scalar>::epsilon());

    const blitzar_core::Vector3 center{0.5 * (minimum.x + maximum.x), 0.5 * (minimum.y + maximum.y),
        0.5 * (minimum.z + maximum.z)};

    cells_.push_back(MakeCell({center, half_extent, 0, particle_count_, 0}));

    for (std::size_t index = 0; index < cells_.size(); ++index) {
        const Cell cell = cells_[index];

        if (cell.count <= leaf_capacity_ || cell.depth >= max_depth_) {
            continue;
        }

        std::array<std::size_t, 8> counts{};
        const blitzar_status status = ExpandCell(index, cell, particles, counts);

        if (status != BLITZAR_STATUS_OK) {
            return status;
        }
    }

    return CalculateProperties(particles);
}

blitzar_status Octree::Build(blitzar_core::ParticleStateView particles) noexcept
{
    cells_.clear();

    particle_count_ = 0;

    blitzar_core::Vector3 minimum{};
    blitzar_core::Vector3 maximum{};
    const blitzar_status preparation_status = PrepareBuild(particles, minimum, maximum);

    if (preparation_status != BLITZAR_STATUS_OK || particle_count_ == 0) {
        return preparation_status;
    }

    const blitzar_status build_status = BuildCells(particles, minimum, maximum);

    if (build_status != BLITZAR_STATUS_OK) {
        cells_.clear();

        particle_count_ = 0;

        return build_status;
    }

    ++build_count_;

    return BLITZAR_STATUS_OK;
}

} // namespace blitzar_trees
