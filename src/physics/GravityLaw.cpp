#include "physics/GravityLaw.hpp"

#include <cmath>

namespace blitzar_physics {

bool GravityParameters::IsValid() const noexcept
{
    return std::isfinite(gravitational_constant) &&
           gravitational_constant > 0.0 && std::isfinite(softening) &&
           softening >= 0.0 && units.IsValid() &&
           std::isfinite(EffectiveConstant()) && EffectiveConstant() > 0.0 &&
           std::isfinite(EffectiveSoftening()) && EffectiveSoftening() >= 0.0;
}

blitzar_core::Scalar GravityParameters::EffectiveConstant() const noexcept
{
    const blitzar_core::Scalar time_squared =
        units.time_scale * units.time_scale;
    const blitzar_core::Scalar length_cubed =
        units.length_scale * units.length_scale * units.length_scale;
    return gravitational_constant * units.mass_scale * time_squared /
           length_cubed;
}

blitzar_core::Scalar GravityParameters::EffectiveSoftening() const noexcept
{
    return softening / units.length_scale;
}

GravityLaw::GravityLaw(GravityParameters parameters) noexcept
    : parameters_(parameters),
      effective_constant_(parameters.EffectiveConstant()),
      effective_softening_(parameters.EffectiveSoftening())
{
}

bool GravityLaw::IsValid() const noexcept
{
    return parameters_.IsValid() && std::isfinite(effective_constant_) &&
           effective_constant_ > 0.0 && std::isfinite(effective_softening_) &&
           effective_softening_ >= 0.0;
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
        squared_distance + effective_softening_ * effective_softening_;
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
        squared_distance + effective_softening_ * effective_softening_;
    return effective_constant_ * source_mass /
           (softened_distance * std::sqrt(softened_distance));
}

}  // namespace blitzar_physics
