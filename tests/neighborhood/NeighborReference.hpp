#ifndef BLITZAR_TESTS_NEIGHBORHOOD_NEIGHBOR_REFERENCE_HPP
#define BLITZAR_TESTS_NEIGHBORHOOD_NEIGHBOR_REFERENCE_HPP

#include "neighborhood/NeighborModel.hpp"

namespace blitzar_neighborhood {

[[nodiscard]] NeighborSet BuildReference(const NeighborFrame& frame, blitzar_core::Scalar radius);
[[nodiscard]] bool AreEqual(const NeighborSet& left, const NeighborSet& right) noexcept;

} // namespace blitzar_neighborhood

#endif
