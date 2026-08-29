#include "neighborhood/NeighborBoundary.hpp"

#include "neighborhood/NeighborCandidate.hpp"
#include "neighborhood/NeighborModel.hpp"
#include "neighborhood/NeighborReference.hpp"

#include <array>
#include <cstddef>

namespace blitzar_neighborhood {

bool CheckBoundary()
{
    NeighborWorkload workload;

    workload.seed = 424242;
    workload.scenario = ScenarioKind::Sparse;
    workload.parameters = {{{-8.0, -8.0, -8.0}, {8.0, 8.0, 8.0}}, 0.75, 0.4, 1};
    workload.positions = {
        {-8.0, -8.0, -8.0}, {8.0, 8.0, 8.0}, {7.25, 8.0, 8.0}, {-8.0, -7.25, -8.0}};

    NeighborFrame frame(workload.positions.size());

    for (std::size_t index = 0; index < workload.positions.size(); ++index) {
        frame.x[index] = workload.positions[index].x;
        frame.y[index] = workload.positions[index].y;
        frame.z[index] = workload.positions[index].z;
    }

    const NeighborSet reference = BuildReference(frame, workload.parameters.radius);
    constexpr std::array<CandidateKind, 4> candidates{CandidateKind::CellLinked,
        CandidateKind::SpatialHash, CandidateKind::HilbertOrder, CandidateKind::Verlet};

    for (const CandidateKind candidate : candidates) {
        NeighborCandidate index(workload, candidate);

        if (!index.Build(frame) || !AreEqual(index.Query(frame), reference)) {
            return false;
        }
    }

    return true;
}

} // namespace blitzar_neighborhood
