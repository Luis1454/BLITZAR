#ifndef BLITZAR_INTEGRATION_COMOVING_COMOVING_LEAPFROG_HPP
#define BLITZAR_INTEGRATION_COMOVING_COMOVING_LEAPFROG_HPP

#include "core/CoreExecution.hpp"
#include "integration/comoving/ComovingBackground.hpp"
#include "integration/kdk/KdkCheckpoint.hpp"
#include "particles/buffer/ParticleAccelerationBuffer.hpp"
#include "particles/buffer/ParticleBuffer.hpp"
#include "solvers/SolverForceEvaluation.hpp"

namespace blitzar_integration_comoving {

template <typename ForceProvider> struct AdvanceState final {
    blitzar_particles::ParticleBuffer& particles;
    blitzar_particles::ParticleAccelerationBuffer& accelerations;
    blitzar_integration::KdkCheckpoint& checkpoint;
    ForceProvider& force_provider;
    blitzar_core::Scalar cosmic_time{};
    blitzar_core::Scalar timestep{};
    const blitzar_integration::ComovingBackground& background;
    const blitzar_core::ExecutionSettings& settings;
    blitzar_core::ParticleStateView solver_particles;
};

} // namespace blitzar_integration_comoving

namespace blitzar_integration {

class ComovingLeapfrog final {
public:
    template <typename ForceProvider>
    [[nodiscard]] blitzar_status Advance(
        blitzar_integration_comoving::AdvanceState<ForceProvider>& state) const noexcept;
};

} // namespace blitzar_integration

#include "integration/comoving/ComovingAdvance.hpp"

#endif
