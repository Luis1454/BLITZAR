#include "neighborhood/NeighborCandidate.hpp"

#include <variant>

namespace blitzar_neighborhood {

NeighborCandidate::Index NeighborCandidate::MakeIndex(
    const NeighborWorkload& workload, CandidateKind candidate)
{
    switch (candidate) {
    case CandidateKind::CellLinked:

        return GridIndex(workload.parameters, GridKind::CellLinked);

    case CandidateKind::SpatialHash:

        return GridIndex(workload.parameters, GridKind::SpatialHash);

    case CandidateKind::HilbertOrder:

        return HilbertIndex(workload.parameters);

    case CandidateKind::Verlet:

        return NeighborVerlet(workload.parameters);
    }

    return GridIndex(workload.parameters, GridKind::CellLinked);
}

NeighborCandidate::NeighborCandidate(const NeighborWorkload& workload, CandidateKind candidate)
    : kind_(candidate), index_(MakeIndex(workload, candidate))
{
}

bool NeighborCandidate::NeedsRebuild(const NeighborFrame& frame) const noexcept
{
    if (kind_ != CandidateKind::Verlet) {
        return false;
    }

    return std::get<NeighborVerlet>(index_).NeedsRebuild(frame);
}

bool NeighborCandidate::Build(const NeighborFrame& frame)
{
    return std::visit([&frame](auto& value) { return value.Build(frame); }, index_);
}

NeighborSet NeighborCandidate::Query(const NeighborFrame& frame) const
{
    return std::visit([&frame](const auto& value) { return value.Query(frame); }, index_);
}

std::size_t NeighborCandidate::MemoryBytes() const noexcept
{
    return std::visit([](const auto& value) { return value.MemoryBytes(); }, index_);
}

} // namespace blitzar_neighborhood
