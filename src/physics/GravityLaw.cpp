#include "physics/GravityLaw.hpp"

#include <cmath>

namespace blitzar_physics {

bool GravityParameters::IsValid() const noexcept
{
    return std::isfinite(gravitational_constant) &&
           gravitational_constant > 0.0 && std::isfinite(softening) &&
           softening >= 0.0;
}

GravityLaw::GravityLaw(GravityParameters parameters) noexcept
    : parameters_(parameters)
{
}

bool GravityLaw::IsValid() const noexcept
{
    return parameters_.IsValid();
}

blitzar_core::Scalar GravityLaw::PairFactor(
    blitzar_core::Scalar source_mass,
    blitzar_core::Scalar squared_distance) const noexcept
{
    const blitzar_core::Scalar softened_distance =
        squared_distance + parameters_.softening * parameters_.softening;
    return parameters_.gravitational_constant * source_mass /
           (softened_distance * std::sqrt(softened_distance));
}

}  // namespace blitzar_physics
