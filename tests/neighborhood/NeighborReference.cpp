#include "neighborhood/NeighborReference.hpp"

#include "neighborhood/NeighborDistance.hpp"

namespace {

bool IsNeighbor(const blitzar_neighborhood::NeighborFrame& frame, std::size_t target,
    std::size_t source, double radius_squared) noexcept
{
    return target != source && blitzar_neighborhood::SquaredDistance(frame.Position(target),
                                   frame.Position(source)) <= radius_squared;
}

} // namespace

namespace blitzar_neighborhood {

NeighborSet BuildReference(const NeighborFrame& frame, blitzar_core::Scalar radius)
{
    const std::size_t count = frame.x.size();
    const double radius_squared = radius * radius;
    NeighborSet result;

    result.offsets.resize(count + 1U);

    for (std::size_t target = 0; target < count; ++target) {
        std::size_t neighbors = 0;

        for (std::size_t source = 0; source < count; ++source) {
            neighbors += IsNeighbor(frame, target, source, radius_squared) ? 1U : 0U;
        }

        result.offsets[target + 1U] = result.offsets[target] + neighbors;
    }

    result.indices.reserve(result.offsets.back());

    for (std::size_t target = 0; target < count; ++target) {
        for (std::size_t source = 0; source < count; ++source) {
            if (IsNeighbor(frame, target, source, radius_squared)) {
                result.indices.push_back(source);
            }
        }
    }

    return result;
}

bool AreEqual(const NeighborSet& left, const NeighborSet& right) noexcept
{
    return left.offsets == right.offsets && left.indices == right.indices;
}

} // namespace blitzar_neighborhood
