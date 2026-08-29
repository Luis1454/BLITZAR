#include "neighborhood/NeighborHilbert.hpp"

#include "neighborhood/NeighborDistance.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace {

constexpr std::uint32_t kBits = 10U;

std::uint32_t Quantize(double value, double minimum, double maximum) noexcept
{
    constexpr std::uint32_t maximum_value = (1U << kBits) - 1U;
    const double normalized = (value - minimum) / (maximum - minimum);
    const auto coordinate = static_cast<std::uint32_t>(std::floor(normalized * maximum_value));

    return std::min(coordinate, maximum_value);
}

std::uint64_t HilbertDistance(std::array<std::uint32_t, 3> axes) noexcept
{
    constexpr std::uint32_t most_significant = 1U << (kBits - 1U);

    for (std::uint32_t q = most_significant; q > 1U; q >>= 1U) {
        const std::uint32_t mask = q - 1U;

        for (std::size_t axis = 0; axis < axes.size(); ++axis) {
            if ((axes[axis] & q) != 0U) {
                axes[0] ^= mask;
            }
            else {
                const std::uint32_t exchange = (axes[0] ^ axes[axis]) & mask;

                axes[0] ^= exchange;
                axes[axis] ^= exchange;
            }
        }
    }
    for (std::size_t axis = 1; axis < axes.size(); ++axis) {
        axes[axis] ^= axes[axis - 1U];
    }

    std::uint32_t gray = 0;

    for (std::uint32_t q = most_significant; q > 1U; q >>= 1U) {
        if ((axes.back() & q) != 0U) {
            gray ^= q - 1U;
        }
    }
    for (std::uint32_t& axis : axes) {
        axis ^= gray;
    }

    std::uint64_t distance = 0;

    for (std::uint32_t bit = kBits; bit > 0U; --bit) {
        const std::uint32_t mask = 1U << (bit - 1U);

        for (const std::uint32_t axis : axes) {
            distance = (distance << 1U) | ((axis & mask) != 0U ? 1U : 0U);
        }
    }

    return distance;
}

} // namespace

namespace blitzar_neighborhood {

HilbertIndex::HilbertIndex(NeighborParameters parameters) : parameters_(parameters) {}

bool HilbertIndex::Build(const NeighborFrame& frame)
{
    if (frame.x.empty() || frame.x.size() != frame.y.size() || frame.x.size() != frame.z.size()) {
        return false;
    }

    entries_.clear();
    entries_.reserve(frame.x.size());

    for (std::size_t particle = 0; particle < frame.x.size(); ++particle) {
        const auto position = frame.Position(particle);
        const std::array<std::uint32_t, 3> axes{
            Quantize(position.x, parameters_.bounds.minimum.x, parameters_.bounds.maximum.x),
            Quantize(position.y, parameters_.bounds.minimum.y, parameters_.bounds.maximum.y),
            Quantize(position.z, parameters_.bounds.minimum.z, parameters_.bounds.maximum.z)};

        entries_.push_back({HilbertDistance(axes), particle});
    }

    std::sort(entries_.begin(), entries_.end(), [](const Entry& left, const Entry& right) noexcept {
        return left.key < right.key || (left.key == right.key && left.particle < right.particle);
    });

    return true;
}

NeighborSet HilbertIndex::Query(const NeighborFrame& frame) const
{
    NeighborSet result;

    result.offsets.resize(frame.x.size() + 1U);

    std::vector<std::size_t> candidates;

    candidates.reserve(32U);

    const double radius_squared = parameters_.radius * parameters_.radius;

    for (std::size_t target = 0; target < frame.x.size(); ++target) {
        candidates.clear();

        for (const Entry& entry : entries_) {
            if (entry.particle != target && SquaredDistance(frame.Position(target),
                                                frame.Position(entry.particle)) <= radius_squared) {
                candidates.push_back(entry.particle);
            }
        }

        std::sort(candidates.begin(), candidates.end());
        result.indices.insert(result.indices.end(), candidates.begin(), candidates.end());

        result.offsets[target + 1U] = result.indices.size();
    }

    return result;
}

std::size_t HilbertIndex::MemoryBytes() const noexcept
{
    return entries_.capacity() * sizeof(Entry);
}

} // namespace blitzar_neighborhood
