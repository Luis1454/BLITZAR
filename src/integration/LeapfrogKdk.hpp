#ifndef BLITZAR_INTEGRATION_LEAPFROG_KDK_HPP
#define BLITZAR_INTEGRATION_LEAPFROG_KDK_HPP

#include "core/Execution.hpp"
#include "core/Solver.hpp"
#include "particles/ParticleBuffer.hpp"

namespace blitzar_integration {

class LeapfrogKdk final {
public:
    [[nodiscard]] blitzar_status Advance(
        blitzar_particles::ParticleBuffer& particles,
        blitzar_particles::AccelerationBuffer& accelerations,
        blitzar_core::Solver& solver,
        blitzar_core::Scalar timestep,
        const blitzar_core::ExecutionSettings& settings) const noexcept;
};

}  // namespace blitzar_integration

#endif
