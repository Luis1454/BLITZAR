#include "integration/LeapfrogKdk.hpp"

#include <cmath>

namespace blitzar_integration {

blitzar_status LeapfrogKdk::Advance(
    blitzar_particles::ParticleBuffer& particles,
    blitzar_particles::AccelerationBuffer& accelerations,
    LeapfrogWorkspace& workspace,
    blitzar_core::Solver& solver,
    blitzar_core::Scalar timestep,
    const blitzar_core::ExecutionSettings& settings) const noexcept
{
    if (!particles.IsValid() || particles.Count() != accelerations.Count() ||
        particles.Count() != workspace.Count() ||
        !std::isfinite(timestep) || timestep <= 0.0 || !settings.IsValid()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    blitzar_core::MutableParticleView mutable_state = particles.MutableView();
    blitzar_status status = workspace.Capture(mutable_state);
    if (status != BLITZAR_STATUS_OK) {
        return status;
    }
    blitzar_core::ForceView force = accelerations.View();
    status = solver.Compute(particles.State(), force, settings);
    if (status != BLITZAR_STATUS_OK) {
        return status;
    }

    const blitzar_core::Scalar half_step = 0.5 * timestep;
    for (std::size_t index = 0; index < particles.Count(); ++index) {
        mutable_state.velocity_x[index] += half_step * force.x[index];
        mutable_state.velocity_y[index] += half_step * force.y[index];
        mutable_state.velocity_z[index] += half_step * force.z[index];
        mutable_state.x[index] += timestep * mutable_state.velocity_x[index];
        mutable_state.y[index] += timestep * mutable_state.velocity_y[index];
        mutable_state.z[index] += timestep * mutable_state.velocity_z[index];
    }

    status = solver.Compute(particles.State(), force, settings);
    if (status != BLITZAR_STATUS_OK) {
        const blitzar_status restore_status = workspace.Restore(mutable_state);
        if (restore_status != BLITZAR_STATUS_OK) {
            return restore_status;
        }
        return status;
    }
    for (std::size_t index = 0; index < particles.Count(); ++index) {
        mutable_state.velocity_x[index] += half_step * force.x[index];
        mutable_state.velocity_y[index] += half_step * force.y[index];
        mutable_state.velocity_z[index] += half_step * force.z[index];
    }
    return BLITZAR_STATUS_OK;
}

}  // namespace blitzar_integration
