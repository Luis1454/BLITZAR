#include "core/Execution.hpp"
#include "integration/LeapfrogKdk.hpp"
#include "physics/GravityLaw.hpp"
#include "particles/ParticleBuffer.hpp"
#include "solvers/direct/DirectSolver.hpp"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>

namespace {

class FailOnSecondSolver final : public blitzar_core::Solver {
public:
    [[nodiscard]] blitzar_core::SolverKind Kind() const noexcept override
    {
        return blitzar_core::SolverKind::Direct;
    }

    [[nodiscard]] blitzar_status Compute(
        blitzar_core::ParticleStateView particles,
        blitzar_core::ForceView forces,
        const blitzar_core::ExecutionSettings& settings) noexcept override
    {
        if (!blitzar_core::IsValid(particles) ||
            !blitzar_core::IsValid(forces) || particles.count != forces.count ||
            !settings.IsValid()) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
        ++calls_;
        if (calls_ == 2) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }
        for (std::size_t index = 0; index < forces.count; ++index) {
            forces.x[index] = 0.0;
            forces.y[index] = 0.0;
            forces.z[index] = 0.0;
        }
        return BLITZAR_STATUS_OK;
    }

private:
    std::size_t calls_{};
};

}  // namespace

int main()
{
    blitzar_particles::ParticleBuffer particles(2);
    blitzar_particles::AccelerationBuffer accelerations(2);
    assert(particles.IsValid());
    assert(particles.Count() == accelerations.Count());
    assert(reinterpret_cast<std::uintptr_t>(particles.State().x) % 64U == 0U);

    assert(particles.SetPosition(0, {0.0, 0.0, 0.0}) == BLITZAR_STATUS_OK);
    assert(particles.SetPosition(1, {1.0, 0.0, 0.0}) == BLITZAR_STATUS_OK);
    assert(particles.SetMass(0, 1.0) == BLITZAR_STATUS_OK);
    assert(particles.SetMass(1, 1.0) == BLITZAR_STATUS_OK);

    const blitzar_core::ExecutionSettings settings{};
    const blitzar_physics::GravityParameters gravity{1.0, 0.0};
    blitzar_direct::DirectSolver solver(gravity);
    assert(solver.Compute(particles.State(), accelerations.View(), settings) ==
           BLITZAR_STATUS_OK);
    const blitzar_core::ForceView force = accelerations.View();
    assert(std::abs(force.x[0] - 1.0) < 1.0e-12);
    assert(std::abs(force.x[1] + 1.0) < 1.0e-12);
    assert(std::abs(force.y[0]) < 1.0e-12);
    assert(std::abs(force.z[1]) < 1.0e-12);

    assert(particles.SetPosition(1, {0.0, 0.0, 0.0}) == BLITZAR_STATUS_OK);
    assert(solver.Compute(particles.State(), accelerations.View(), settings) ==
           BLITZAR_STATUS_SINGULARITY);
    assert(particles.SetPosition(1, {1.0, 0.0, 0.0}) == BLITZAR_STATUS_OK);
    assert(particles.SetMass(1, std::numeric_limits<double>::quiet_NaN()) ==
           BLITZAR_STATUS_INVALID_ARGUMENT);
    assert(solver.Compute(particles.State(), accelerations.View(), settings) ==
           BLITZAR_STATUS_OK);
    assert(particles.SetMass(1, 1.0) == BLITZAR_STATUS_OK);

    blitzar_particles::ParticleBuffer free_particle(1);
    blitzar_particles::AccelerationBuffer free_acceleration(1);
    blitzar_integration::LeapfrogWorkspace free_workspace(1);
    assert(free_particle.SetVelocity(0, {2.0, 0.0, 0.0}) == BLITZAR_STATUS_OK);
    assert(free_particle.SetPosition(1, {0.0, 0.0, 0.0}) ==
           BLITZAR_STATUS_INVALID_ARGUMENT);
    const blitzar_integration::LeapfrogKdk integrator{};
    assert(integrator.Advance(
               free_particle,
               free_acceleration,
               free_workspace,
               solver,
               0.5,
               settings) == BLITZAR_STATUS_OK);
    const blitzar_core::ParticleStateView state = free_particle.State();
    assert(std::abs(state.x[0] - 1.0) < 1.0e-12);
    assert(std::abs(state.velocity_x[0] - 2.0) < 1.0e-12);
    assert(integrator.Advance(
               free_particle,
               free_acceleration,
               free_workspace,
               solver,
               0.0,
               settings) == BLITZAR_STATUS_INVALID_ARGUMENT);

    blitzar_particles::ParticleBuffer rollback_particle(1);
    blitzar_particles::AccelerationBuffer rollback_acceleration(1);
    blitzar_integration::LeapfrogWorkspace rollback_workspace(1);
    assert(rollback_particle.SetVelocity(0, {1.0, 0.0, 0.0}) ==
           BLITZAR_STATUS_OK);
    FailOnSecondSolver failing_solver{};
    assert(integrator.Advance(
               rollback_particle,
               rollback_acceleration,
               rollback_workspace,
               failing_solver,
               0.5,
               settings) == BLITZAR_STATUS_INTERNAL_ERROR);
    const blitzar_core::ParticleStateView restored = rollback_particle.State();
    assert(std::abs(restored.x[0]) < 1.0e-12);
    assert(std::abs(restored.velocity_x[0] - 1.0) < 1.0e-12);
    return 0;
}
