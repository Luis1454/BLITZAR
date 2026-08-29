#ifndef BLITZAR_TESTS_NEIGHBORHOOD_NEIGHBOR_RUN_HPP
#define BLITZAR_TESTS_NEIGHBORHOOD_NEIGHBOR_RUN_HPP

#include "neighborhood/NeighborModel.hpp"

namespace blitzar_neighborhood {

[[nodiscard]] CandidateRun MeasureCandidate(
    const NeighborWorkload& workload, CandidateKind candidate);

} // namespace blitzar_neighborhood

#endif
