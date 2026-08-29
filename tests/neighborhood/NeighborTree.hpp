#ifndef BLITZAR_TESTS_NEIGHBORHOOD_NEIGHBOR_TREE_HPP
#define BLITZAR_TESTS_NEIGHBORHOOD_NEIGHBOR_TREE_HPP

#include "neighborhood/NeighborModel.hpp"

namespace blitzar_neighborhood {

[[nodiscard]] TreeMetrics MeasureTree(const NeighborWorkload& workload);

} // namespace blitzar_neighborhood

#endif
