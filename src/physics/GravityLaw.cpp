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

PairStatus GravityLaw::ValidatePair(
    blitzar_core::Scalar source_mass,
    blitzar_core::Scalar squared_distance) const noexcept
{
    if (!IsValid() || !std::isfinite(source_mass) || source_mass < 0.0 ||
        !std::isfinite(squared_distance) || squared_distance < 0.0) {
        return PairStatus::Invalid;
    }
    const blitzar_core::Scalar softened_distance =
        squared_distance + parameters_.softening * parameters_.softening;
    if (!std::isfinite(softened_distance)) {
        return PairStatus::Invalid;
    }
    if (softened_distance == 0.0) {
        return PairStatus::Singularity;
    }
    return PairStatus::Valid;
}

bool GravityLaw::IsValidPair(
    blitzar_core::Scalar source_mass,
    blitzar_core::Scalar squared_distance) const noexcept
{
    return ValidatePair(source_mass, squared_distance) == PairStatus::Valid;
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
