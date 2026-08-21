#include "core/Execution.hpp"
#include "integration/LeapfrogKdk.hpp"
#include "physics/GravityLaw.hpp"
#include "particles/ParticleBuffer.hpp"
#include "solvers/direct/DirectSolver.hpp"

#include <cassert>
#include <cmath>
#include <cstdint>

int main()
{
    blitzar_particles::ParticleBuffer particles(2);
    blitzar_particles::AccelerationBuffer accelerations(2);
    assert(particles.IsValid());
    assert(particles.Count() == accelerations.Count());
    assert(reinterpret_cast<std::uintptr_t>(particles.State().x) % 64U == 0U);

    particles.SetPosition(0, {0.0, 0.0, 0.0});
    particles.SetPosition(1, {1.0, 0.0, 0.0});
    particles.SetMass(0, 1.0);
    particles.SetMass(1, 1.0);

    const blitzar_core::ExecutionSettings settings{};
    const blitzar_physics::GravityParameters gravity{1.0, 0.0};
    const blitzar_direct::DirectSolver solver(gravity);
    assert(solver.Compute(particles.State(), accelerations.View(), settings) ==
           BLITZAR_STATUS_OK);
    const blitzar_core::ForceView force = accelerations.View();
    assert(std::abs(force.x[0] - 1.0) < 1.0e-12);
    assert(std::abs(force.x[1] + 1.0) < 1.0e-12);
    assert(std::abs(force.y[0]) < 1.0e-12);
    assert(std::abs(force.z[1]) < 1.0e-12);

    blitzar_particles::ParticleBuffer free_particle(1);
    blitzar_particles::AccelerationBuffer free_acceleration(1);
    free_particle.SetVelocity(0, {2.0, 0.0, 0.0});
    const blitzar_integration::LeapfrogKdk integrator{};
    assert(integrator.Advance(
               free_particle,
               free_acceleration,
               solver,
               0.5,
               settings) == BLITZAR_STATUS_OK);
    const blitzar_core::ParticleStateView state = free_particle.State();
    assert(std::abs(state.x[0] - 1.0) < 1.0e-12);
    assert(std::abs(state.velocity_x[0] - 2.0) < 1.0e-12);
    assert(integrator.Advance(
               free_particle,
               free_acceleration,
               solver,
               0.0,
               settings) == BLITZAR_STATUS_INVALID_ARGUMENT);
    return 0;
}
