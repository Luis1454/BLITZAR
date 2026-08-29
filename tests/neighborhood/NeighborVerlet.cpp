#include "neighborhood/NeighborVerlet.hpp"

#include "neighborhood/NeighborDistance.hpp"
#include "neighborhood/NeighborReference.hpp"

namespace blitzar_neighborhood {

NeighborVerlet::NeighborVerlet(NeighborParameters parameters) : parameters_(parameters) {}

bool NeighborVerlet::Build(const NeighborFrame& frame)
{
    if (!blitzar_core::IsValid(frame.View())) {
        return false;
    }

    reference_positions_.clear();
    reference_positions_.reserve(frame.x.size());

    for (std::size_t index = 0; index < frame.x.size(); ++index) {
        reference_positions_.push_back(frame.Position(index));
    }

    candidates_ = BuildReference(frame, parameters_.radius + parameters_.skin);

    return candidates_.IsValid(frame.x.size());
}

bool NeighborVerlet::NeedsRebuild(const NeighborFrame& frame) const noexcept
{
    if (reference_positions_.size() != frame.x.size()) {
        return true;
    }

    const double threshold = parameters_.skin * 0.5;
    const double threshold_squared = threshold * threshold;

    for (std::size_t index = 0; index < frame.x.size(); ++index) {
        if (SquaredDistance(reference_positions_[index], frame.Position(index)) >
            threshold_squared) {
            return true;
        }
    }

    return false;
}

NeighborSet NeighborVerlet::Query(const NeighborFrame& frame) const
{
    NeighborSet result;

    result.offsets.resize(frame.x.size() + 1U);

    const double radius_squared = parameters_.radius * parameters_.radius;

    for (std::size_t target = 0; target < frame.x.size(); ++target) {
        const std::size_t begin = candidates_.offsets[target];
        const std::size_t end = candidates_.offsets[target + 1U];

        for (std::size_t offset = begin; offset < end; ++offset) {
            const std::size_t source = candidates_.indices[offset];

            if (SquaredDistance(frame.Position(target), frame.Position(source)) <= radius_squared) {
                result.indices.push_back(source);
            }
        }

        result.offsets[target + 1U] = result.indices.size();
    }

    return result;
}

std::size_t NeighborVerlet::MemoryBytes() const noexcept
{
    return reference_positions_.capacity() * sizeof(blitzar_core::Vector3) +
           candidates_.MemoryBytes();
}

} // namespace blitzar_neighborhood
