#include "trees/Octree.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

#if defined(_OPENMP)
#include <omp.h>
#endif

namespace blitzar_trees {

namespace {

constexpr std::size_t ParallelMortonThreshold = 1'000'000;
} // namespace

bool Octree::Cell::IsLeaf() const noexcept
{
    return std::all_of(children.begin(), children.end(),
        [](const std::size_t child) { return child == Cell::InvalidIndex; });
}

Octree::Octree(std::size_t max_particles, std::size_t max_cells, std::size_t leaf_capacity,
    std::size_t max_depth)
    : max_particles_(max_particles), max_cells_(max_cells), leaf_capacity_(leaf_capacity),
      max_depth_(max_depth), particle_count_(0), build_count_(0), refit_count_(0),
      indices_(max_particles), scratch_(max_particles), morton_keys_(max_particles)
{
    cells_.reserve(max_cells);
}

bool Octree::IsValidInput(blitzar_core::ParticleStateView particles) noexcept
{
    if (!blitzar_core::IsValid(particles)) {
        return false;
    }

    for (std::size_t index = 0; index < particles.SourceCount(); ++index) {
        if (!std::isfinite(particles.x[index]) || !std::isfinite(particles.y[index]) ||
            !std::isfinite(particles.z[index]) || !std::isfinite(particles.mass[index]) ||
            particles.mass[index] < 0.0) {
            return false;
        }
    }

    return true;
}

Octree::Cell Octree::MakeCell(CellPlacement placement) const noexcept
{
    Cell cell{};

    cell.center = placement.center;
    cell.half_extent = placement.half_extent;
    cell.begin = placement.begin;
    cell.count = placement.count;
    cell.depth = placement.depth;

    cell.children.fill(Cell::InvalidIndex);

    return cell;
}

std::size_t Octree::Octant(const Cell& cell, blitzar_core::Vector3 position) noexcept
{
    return (position.x >= cell.center.x ? std::size_t{1} : std::size_t{0}) |
           (position.y >= cell.center.y ? std::size_t{2} : std::size_t{0}) |
           (position.z >= cell.center.z ? std::size_t{4} : std::size_t{0});
}

bool Octree::Contains(const Cell& cell, blitzar_core::Vector3 position) noexcept
{
    return std::abs(position.x - cell.center.x) <= cell.half_extent &&
           std::abs(position.y - cell.center.y) <= cell.half_extent &&
           std::abs(position.z - cell.center.z) <= cell.half_extent;
}

void Octree::Partition(const Cell& cell, blitzar_core::ParticleStateView particles,
    std::array<std::size_t, 8>& counts) noexcept
{
    counts.fill(0);

    for (std::size_t offset = 0; offset < cell.count; ++offset) {
        const std::size_t particle = indices_[cell.begin + offset];

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
}

void Octree::CalculateProperties(blitzar_core::ParticleStateView particles) noexcept
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
        }
        else {
            for (const std::size_t child : cell.children) {
                if (child == Cell::InvalidIndex) {
                    continue;
                }

                const Cell& child_cell = cells_[child];
                cell.mass += child_cell.mass;
                cell.center_of_mass.x += child_cell.mass * child_cell.center_of_mass.x;
                cell.center_of_mass.y += child_cell.mass * child_cell.center_of_mass.y;
                cell.center_of_mass.z += child_cell.mass * child_cell.center_of_mass.z;
            }
        }
        if (cell.mass > 0.0) {
            cell.center_of_mass.x /= cell.mass;
            cell.center_of_mass.y /= cell.mass;
            cell.center_of_mass.z /= cell.mass;
        }
        else {
            cell.center_of_mass = cell.center;
        }
    }
}

void Octree::ParallelMortonSort(blitzar_core::ParticleStateView particles,
    blitzar_core::Vector3 minimum, blitzar_core::Vector3 maximum) noexcept
{
    if (particles.SourceCount() <= ParallelMortonThreshold) {
        return;
    }

#if defined(_OPENMP)

    const int available_threads = omp_get_max_threads();
    const std::size_t thread_count =
        available_threads > 0
            ? std::min(static_cast<std::size_t>(available_threads), particles.SourceCount())
            : std::size_t{1};
#else

    constexpr std::size_t thread_count = 1;
#endif
    const std::size_t chunk_size = particles.SourceCount() / thread_count +
                                   (particles.SourceCount() % thread_count == 0 ? 0 : 1);

#if defined(_OPENMP)
#pragma omp parallel for schedule(static)
#endif

    for (std::int64_t raw_chunk = 0; raw_chunk < static_cast<std::int64_t>(thread_count);
         ++raw_chunk) {
        const std::size_t chunk = static_cast<std::size_t>(raw_chunk);
        const std::size_t begin = chunk * chunk_size;

        if (begin >= particles.SourceCount()) {
            continue;
        }

        const std::size_t end = begin + std::min(chunk_size, particles.SourceCount() - begin);

        for (std::size_t index = begin; index < end; ++index) {
            morton_keys_[index] = MortonKey(
                {particles.x[index], particles.y[index], particles.z[index]}, minimum, maximum);
        }

        std::sort(indices_.begin() + static_cast<std::ptrdiff_t>(begin),
            indices_.begin() + static_cast<std::ptrdiff_t>(end),
            [this](const std::size_t lhs, const std::size_t rhs) noexcept {
                return morton_keys_[lhs] < morton_keys_[rhs] ||
                       (morton_keys_[lhs] == morton_keys_[rhs] && lhs < rhs);
            });
    }

    for (std::size_t width = chunk_size; width < particles.SourceCount();) {
        const std::size_t pair_width =
            width > particles.SourceCount() - width ? particles.SourceCount() : width + width;

        const std::size_t pair_count = 1 + (particles.SourceCount() - 1) / pair_width;
#if defined(_OPENMP)
#pragma omp parallel for schedule(static)
#endif

        for (std::int64_t raw_pair = 0; raw_pair < static_cast<std::int64_t>(pair_count);
             ++raw_pair) {
            const std::size_t pair = static_cast<std::size_t>(raw_pair);
            const std::size_t begin = pair * pair_width;
            const std::size_t middle = begin + std::min(width, particles.SourceCount() - begin);
            const std::size_t end = begin + std::min(pair_width, particles.SourceCount() - begin);

            std::merge(indices_.begin() + static_cast<std::ptrdiff_t>(begin),
                indices_.begin() + static_cast<std::ptrdiff_t>(middle),
                indices_.begin() + static_cast<std::ptrdiff_t>(middle),
                indices_.begin() + static_cast<std::ptrdiff_t>(end),
                scratch_.begin() + static_cast<std::ptrdiff_t>(begin),
                [this](const std::size_t lhs, const std::size_t rhs) noexcept {
                    return morton_keys_[lhs] < morton_keys_[rhs] ||
                           (morton_keys_[lhs] == morton_keys_[rhs] && lhs < rhs);
                });
        }
#if defined(_OPENMP)
#pragma omp parallel for schedule(static)
#endif
        for (std::int64_t raw_index = 0;
             raw_index < static_cast<std::int64_t>(particles.SourceCount()); ++raw_index) {
            const std::size_t index = static_cast<std::size_t>(raw_index);

            indices_[index] = scratch_[index];
        }

        if (width > particles.SourceCount() / 2) {
            break;
        }

        width += width;
    }
}

blitzar_status Octree::Build(blitzar_core::ParticleStateView particles) noexcept
{
    cells_.clear();

    particle_count_ = 0;

    if (particles.SourceCount() > max_particles_ || leaf_capacity_ == 0 || max_cells_ == 0 ||
        !IsValidInput(particles)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    if (particles.SourceCount() == 0) {
        return BLITZAR_STATUS_OK;
    }

    particle_count_ = particles.SourceCount();

    blitzar_core::Vector3 minimum{particles.x[0], particles.y[0], particles.z[0]};
    blitzar_core::Vector3 maximum = minimum;

    for (std::size_t index = 0; index < particles.SourceCount(); ++index) {
        minimum.x = std::min(minimum.x, particles.x[index]);
        minimum.y = std::min(minimum.y, particles.y[index]);
        minimum.z = std::min(minimum.z, particles.z[index]);
        maximum.x = std::max(maximum.x, particles.x[index]);
        maximum.y = std::max(maximum.y, particles.y[index]);
        maximum.z = std::max(maximum.z, particles.z[index]);
        indices_[index] = index;
    }

    ParallelMortonSort(particles, minimum, maximum);

    const blitzar_core::Scalar span =
        std::max({maximum.x - minimum.x, maximum.y - minimum.y, maximum.z - minimum.z});

    const blitzar_core::Scalar half_extent =
        std::max(0.5 * span, std::numeric_limits<blitzar_core::Scalar>::epsilon());

    const blitzar_core::Vector3 center{0.5 * (minimum.x + maximum.x), 0.5 * (minimum.y + maximum.y),
        0.5 * (minimum.z + maximum.z)};

    cells_.push_back(MakeCell({center, half_extent, 0, particles.SourceCount(), 0}));

    for (std::size_t index = 0; index < cells_.size(); ++index) {
        const Cell cell = cells_[index];

        if (cell.count <= leaf_capacity_ || cell.depth >= max_depth_) {
            continue;
        }

        std::array<std::size_t, 8> counts{};

        Partition(cell, particles, counts);

        std::size_t child_count = 0;

        for (const std::size_t count : counts) {
            if (count > 0) {
                ++child_count;
            }
        }

        if (cells_.size() > max_cells_ || child_count > max_cells_ - cells_.size()) {
            cells_.clear();

            particle_count_ = 0;

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

            cells_.push_back(MakeCell(
                {child_center, child_half_extent, begin, counts[octant], cell.depth + 1}));

            cells_[index].children[octant] = child;
        }
    }

    CalculateProperties(particles);

    ++build_count_;

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
            const std::size_t particle = indices_[cell.begin + offset];
            const blitzar_core::Vector3 position{
                particles.x[particle], particles.y[particle], particles.z[particle]};

            if (!Contains(cell, position)) {
                return false;
            }
        }
    }

    CalculateProperties(particles);

    ++refit_count_;

    return true;
}

std::size_t Octree::CellCount() const noexcept
{
    return cells_.size();
}

std::size_t Octree::ParticleCount() const noexcept
{
    return particle_count_;
}

std::size_t Octree::BuildCount() const noexcept
{
    return build_count_;
}

std::size_t Octree::RefitCount() const noexcept
{
    return refit_count_;
}

std::span<const Octree::Cell> Octree::Cells() const noexcept
{
    return std::span<const Cell>(cells_);
}

std::span<const std::size_t> Octree::Indices() const noexcept
{
    if (particle_count_ == 0) {
        return {};
    }

    return std::span<const std::size_t>(indices_).first(particle_count_);
}

std::span<const Octree::Cell> Octree::CellAt(std::size_t index) const noexcept
{
    if (index >= cells_.size()) {
        return {};
    }

    return std::span<const Cell>(cells_).subspan(index, 1);
}

bool Octree::ParticleIndex(std::size_t index, std::size_t& particle) const noexcept
{
    if (index >= particle_count_) {
        return false;
    }

    particle = indices_[index];

    return particle < particle_count_;
}

} // namespace blitzar_trees
