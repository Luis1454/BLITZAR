#include "neighborhood/NeighborGrid.hpp"

#include "neighborhood/NeighborDistance.hpp"

#include <algorithm>
#include <cmath>

namespace {

bool Before(const blitzar_neighborhood::GridEntry& left,
    const blitzar_neighborhood::GridEntry& right) noexcept
{
    if (left.key != right.key) {
        return left.key < right.key;
    }
    if (left.cell.x != right.cell.x) {
        return left.cell.x < right.cell.x;
    }
    if (left.cell.y != right.cell.y) {
        return left.cell.y < right.cell.y;
    }
    if (left.cell.z != right.cell.z) {
        return left.cell.z < right.cell.z;
    }

    return left.particle < right.particle;
}

std::uint64_t CellKey(blitzar_neighborhood::GridCell cell) noexcept
{
    constexpr std::uint64_t radix = 2048U;

    return (static_cast<std::uint64_t>(cell.x) * radix + static_cast<std::uint64_t>(cell.y)) *
               radix +
           static_cast<std::uint64_t>(cell.z);
}

std::uint64_t HashKey(blitzar_neighborhood::GridCell cell) noexcept
{
    std::uint64_t hash = 1469598103934665603ULL;

    hash ^= static_cast<std::uint64_t>(cell.x);
    hash *= 1099511628211ULL;
    hash ^= static_cast<std::uint64_t>(cell.y);
    hash *= 1099511628211ULL;
    hash ^= static_cast<std::uint64_t>(cell.z);
    hash *= 1099511628211ULL;

    return hash;
}

} // namespace

namespace blitzar_neighborhood {

GridIndex::GridIndex(NeighborParameters parameters, GridKind kind)
    : parameters_(parameters), kind_(kind),
      dimensions_{std::max(1, static_cast<int>(std::ceil(
                                  (parameters.bounds.maximum.x - parameters.bounds.minimum.x) /
                                  parameters.radius))),
          std::max(1, static_cast<int>(
                          std::ceil((parameters.bounds.maximum.y - parameters.bounds.minimum.y) /
                                    parameters.radius))),
          std::max(1, static_cast<int>(
                          std::ceil((parameters.bounds.maximum.z - parameters.bounds.minimum.z) /
                                    parameters.radius)))}
{
}

bool GridIndex::Build(const NeighborFrame& frame)
{
    if (frame.x.empty() || frame.x.size() != frame.y.size() || frame.x.size() != frame.z.size()) {
        return false;
    }

    entries_.clear();
    entries_.reserve(frame.x.size());

    for (std::size_t particle = 0; particle < frame.x.size(); ++particle) {
        const GridCell cell = Locate(frame.Position(particle));

        entries_.push_back({cell, Key(cell), particle});
    }

    std::sort(entries_.begin(), entries_.end(), Before);
    BuildRanges();

    return true;
}

std::size_t GridIndex::MemoryBytes() const noexcept
{
    return entries_.capacity() * sizeof(GridEntry) + ranges_.capacity() * sizeof(GridRange);
}

GridCell GridIndex::Locate(blitzar_core::Vector3 position) const noexcept
{
    const auto axis = [](double coordinate, double minimum, double radius, int dimension) noexcept {
        const int value = static_cast<int>(std::floor((coordinate - minimum) / radius));

        return std::clamp(value, 0, dimension - 1);
    };

    return {axis(position.x, parameters_.bounds.minimum.x, parameters_.radius, dimensions_.x),
        axis(position.y, parameters_.bounds.minimum.y, parameters_.radius, dimensions_.y),
        axis(position.z, parameters_.bounds.minimum.z, parameters_.radius, dimensions_.z)};
}

std::uint64_t GridIndex::Key(GridCell cell) const noexcept
{
    return kind_ == GridKind::CellLinked ? CellKey(cell) : HashKey(cell);
}

} // namespace blitzar_neighborhood
