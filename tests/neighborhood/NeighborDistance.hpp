#ifndef BLITZAR_TESTS_NEIGHBORHOOD_NEIGHBOR_DISTANCE_HPP
#define BLITZAR_TESTS_NEIGHBORHOOD_NEIGHBOR_DISTANCE_HPP

#include "core/CoreTypes.hpp"

namespace blitzar_neighborhood {

[[nodiscard]] inline blitzar_core::Scalar SquaredDistance(
    blitzar_core::Vector3 left, blitzar_core::Vector3 right) noexcept
{
    const blitzar_core::Scalar x = left.x - right.x;
    const blitzar_core::Scalar y = left.y - right.y;
    const blitzar_core::Scalar z = left.z - right.z;

    return x * x + y * y + z * z;
}

} // namespace blitzar_neighborhood

#endif
