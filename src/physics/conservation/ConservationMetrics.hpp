#ifndef BLITZAR_PHYSICS_CONSERVATION_CONSERVATION_METRICS_HPP
#define BLITZAR_PHYSICS_CONSERVATION_CONSERVATION_METRICS_HPP

#include "core/CoreTypes.hpp"
#include "physics/gravity/GravityLaw.hpp"
#include "physics/reduction/ScalarReduction.hpp"

#include <blitzar/blitzar.h>

namespace blitzar_physics {

struct ConservationMetrics final {
    blitzar_core::Scalar kinetic_energy{};
    blitzar_core::Scalar potential_energy{};
    blitzar_core::Scalar total_energy{};
    blitzar_core::Vector3 momentum{};

    [[nodiscard]] bool IsFinite() const noexcept;
};

// Metrics operate on a complete local state; extended source views are rejected.
[[nodiscard]] blitzar_status ComputeConservationMetrics(blitzar_core::ParticleStateView state,
    GravityParameters parameters, ConservationMetrics& metrics) noexcept;

[[nodiscard]] blitzar_status ComputeConservationMetrics(blitzar_core::ParticleStateView state,
    GravityParameters parameters, ReductionKind reduction, ConservationMetrics& metrics) noexcept;

} // namespace blitzar_physics

#endif
