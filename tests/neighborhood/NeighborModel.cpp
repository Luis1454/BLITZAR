#include "neighborhood/NeighborModel.hpp"

#include <bit>
#include <cmath>

namespace {

std::uint64_t Mix(std::uint64_t hash, std::uint64_t value) noexcept
{
    hash ^= value;
    hash *= 1099511628211ULL;

    return hash;
}

bool IsFinite(blitzar_core::Vector3 value) noexcept
{
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

std::uint64_t HashValues(std::span<const std::size_t> values) noexcept
{
    std::uint64_t hash = 1469598103934665603ULL;

    for (const std::size_t value : values) {
        hash = Mix(hash, static_cast<std::uint64_t>(value));
    }

    return hash;
}

} // namespace

namespace blitzar_neighborhood {

bool NeighborParameters::IsValid() const noexcept
{
    return IsFinite(bounds.minimum) && IsFinite(bounds.maximum) &&
           bounds.minimum.x < bounds.maximum.x && bounds.minimum.y < bounds.maximum.y &&
           bounds.minimum.z < bounds.maximum.z && std::isfinite(radius) && radius > 0.0 &&
           std::isfinite(skin) && skin > 0.0 && steps > 0;
}

NeighborFrame::NeighborFrame(std::size_t count)
    : x(count), y(count), z(count), velocity_x(count), velocity_y(count), velocity_z(count),
      mass(count, 1.0)
{
}

blitzar_core::ParticleStateView NeighborFrame::View() const noexcept
{
    return {x.size(), x, y, z, velocity_x, velocity_y, velocity_z, mass, x.size()};
}

blitzar_core::Vector3 NeighborFrame::Position(std::size_t index) const noexcept
{
    return {x[index], y[index], z[index]};
}

bool NeighborSet::IsValid(std::size_t particle_count) const noexcept
{
    if (offsets.size() != particle_count + 1 || offsets.empty() || offsets.front() != 0 ||
        offsets.back() != indices.size()) {
        return false;
    }

    for (std::size_t index = 1; index < offsets.size(); ++index) {
        if (offsets[index - 1] > offsets[index]) {
            return false;
        }
    }
    for (const std::size_t index : indices) {
        if (index >= particle_count) {
            return false;
        }
    }

    return true;
}

std::size_t NeighborSet::Count() const noexcept
{
    return indices.size();
}

std::size_t NeighborSet::MemoryBytes() const noexcept
{
    return offsets.capacity() * sizeof(std::size_t) + indices.capacity() * sizeof(std::size_t);
}

std::uint64_t NeighborSet::Hash() const noexcept
{
    std::uint64_t hash = HashValues(offsets);

    hash = Mix(hash, static_cast<std::uint64_t>(indices.size()));

    for (const std::size_t value : indices) {
        hash = Mix(hash, static_cast<std::uint64_t>(value));
    }

    return hash;
}

std::uint64_t NeighborSet::OrderingHash() const noexcept
{
    return HashValues(indices);
}

} // namespace blitzar_neighborhood
