#include "trees/Octree.hpp"

#include <algorithm>
#include <cstdint>

#if defined(_OPENMP)
#include <omp.h>
#endif

namespace blitzar_trees {

namespace {

constexpr std::size_t ParallelMortonThreshold = 1'000'000;

} // namespace

std::size_t Octree::SortMortonChunks(blitzar_core::ParticleStateView particles,
    blitzar_core::Vector3 minimum, blitzar_core::Vector3 maximum) noexcept
{
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

    return chunk_size;
}

void Octree::CopyMortonScratch(std::size_t particle_count) noexcept
{
#if defined(_OPENMP)
#pragma omp parallel for schedule(static)
#endif
    for (std::int64_t raw_index = 0; raw_index < static_cast<std::int64_t>(particle_count);
        ++raw_index) {
        const std::size_t index = static_cast<std::size_t>(raw_index);

        indices_[index] = scratch_[index];
    }
}

void Octree::MergeMortonWidth(std::size_t particle_count, std::size_t width) noexcept
{
    const std::size_t pair_width = width > particle_count - width ? particle_count : width + width;

    const std::size_t pair_count = 1 + (particle_count - 1) / pair_width;

#if defined(_OPENMP)
#pragma omp parallel for schedule(static)
#endif

    for (std::int64_t raw_pair = 0; raw_pair < static_cast<std::int64_t>(pair_count); ++raw_pair) {
        const std::size_t pair = static_cast<std::size_t>(raw_pair);
        const std::size_t begin = pair * pair_width;
        const std::size_t middle = begin + std::min(width, particle_count - begin);
        const std::size_t end = begin + std::min(pair_width, particle_count - begin);

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

    CopyMortonScratch(particle_count);
}

void Octree::MergeMortonChunks(std::size_t particle_count, std::size_t chunk_size) noexcept
{
    for (std::size_t width = chunk_size; width < particle_count;) {
        MergeMortonWidth(particle_count, width);

        if (width > particle_count / 2) {
            break;
        }

        width += width;
    }
}

void Octree::ParallelMortonSort(blitzar_core::ParticleStateView particles,
    blitzar_core::Vector3 minimum, blitzar_core::Vector3 maximum) noexcept
{
    if (particles.SourceCount() <= ParallelMortonThreshold) {
        return;
    }

    const std::size_t chunk_size = SortMortonChunks(particles, minimum, maximum);

    MergeMortonChunks(particles.SourceCount(), chunk_size);
}

} // namespace blitzar_trees
