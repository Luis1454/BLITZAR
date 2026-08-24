#include "Check.hpp"
#include "core/Execution.hpp"
#include "integration/LeapfrogKdk.hpp"
#include "particles/ParticleBuffer.hpp"
#include "physics/GravityLaw.hpp"
#include "solvers/direct/DirectSolver.hpp"

#include <cmath>
#include <cstdint>
#include <limits>
#include <span>
#include <utility>

namespace {

class FailOnSecondSolver final {
public:
    [[nodiscard]] blitzar_core::SolverKind Kind() const noexcept
    {
        return blitzar_core::SolverKind::Direct;
    }

    [[nodiscard]] blitzar_status Compute(blitzar_core::ParticleStateView particles,
        blitzar_core::ForceView forces, const blitzar_core::ExecutionSettings& settings) noexcept
    {
        if (!blitzar_core::IsValid(particles) || !blitzar_core::IsValid(forces) ||
            particles.count != forces.count || !settings.IsValid()) {
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

class NonFiniteSolver final {
public:
    [[nodiscard]] blitzar_core::SolverKind Kind() const noexcept
    {
        return blitzar_core::SolverKind::Direct;
    }

    [[nodiscard]] blitzar_status Compute(blitzar_core::ParticleStateView particles,
        blitzar_core::ForceView forces, const blitzar_core::ExecutionSettings& settings) noexcept
    {
        if (!blitzar_core::IsValid(particles) || !blitzar_core::IsValid(forces) ||
            particles.count != forces.count || !settings.IsValid()) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        for (std::size_t index = 0; index < forces.count; ++index) {
            forces.x[index] = std::numeric_limits<double>::infinity();
            forces.y[index] = 0.0;
            forces.z[index] = 0.0;
        }

        return BLITZAR_STATUS_OK;
    }
};

} // namespace

int main()
{
    {
        blitzar_particles::ParticleBuffer source(1);
        blitzar_particles::ParticleBuffer moved(std::move(source));

        BLITZAR_CHECK(source.Count() == 0);
        BLITZAR_CHECK(source.IsValid());
        BLITZAR_CHECK(moved.Count() == 1);
        BLITZAR_CHECK(moved.IsValid());
        BLITZAR_CHECK(source.SetPosition(0, {1.0, 0.0, 0.0}) == BLITZAR_STATUS_INVALID_ARGUMENT);

        blitzar_particles::ParticleBuffer assigned(0);

        assigned = std::move(moved);

        BLITZAR_CHECK(moved.Count() == 0);
        BLITZAR_CHECK(moved.IsValid());
        BLITZAR_CHECK(assigned.Count() == 1);
        BLITZAR_CHECK(assigned.IsValid());

        blitzar_particles::AccelerationBuffer acceleration_source(1);
        blitzar_particles::AccelerationBuffer acceleration_moved(std::move(acceleration_source));

        BLITZAR_CHECK(acceleration_source.Count() == 0);
        BLITZAR_CHECK(acceleration_source.IsValid());
        BLITZAR_CHECK(acceleration_moved.IsValid());

        blitzar_integration::KdkCheckpoint checkpoint_source(1);
        blitzar_integration::KdkCheckpoint checkpoint_moved(std::move(checkpoint_source));

        BLITZAR_CHECK(checkpoint_source.Count() == 0);
        BLITZAR_CHECK(checkpoint_source.IsValid());
        BLITZAR_CHECK(checkpoint_moved.IsValid());
    }

    {
        blitzar_particles::ParticleArena arena(2);
        blitzar_particles::ParticleBuffer particles(arena);
        blitzar_particles::AccelerationBuffer accelerations(arena);
        blitzar_integration::KdkCheckpoint checkpoint(arena);

        BLITZAR_CHECK(arena.IsValid());
        BLITZAR_CHECK(particles.IsValid());
        BLITZAR_CHECK(accelerations.IsValid());
        BLITZAR_CHECK(checkpoint.IsValid());

        blitzar_particles::ParticleBuffer moved_particles(std::move(particles));
        blitzar_particles::AccelerationBuffer moved_accelerations(std::move(accelerations));
        blitzar_integration::KdkCheckpoint moved_checkpoint(std::move(checkpoint));

        BLITZAR_CHECK(particles.Count() == 0);
        BLITZAR_CHECK(accelerations.Count() == 0);
        BLITZAR_CHECK(checkpoint.Count() == 0);
        BLITZAR_CHECK(moved_particles.IsValid());
        BLITZAR_CHECK(moved_accelerations.IsValid());
        BLITZAR_CHECK(moved_checkpoint.IsValid());
        BLITZAR_CHECK(arena.IsValid());
    }

    blitzar_particles::ParticleBuffer particles(2);

    blitzar_particles::AccelerationBuffer accelerations(2);

    BLITZAR_CHECK(particles.IsValid());
    BLITZAR_CHECK(particles.Count() == accelerations.Count());
    BLITZAR_CHECK(reinterpret_cast<std::uintptr_t>(particles.State().x.data()) % 64U == 0U);
    BLITZAR_CHECK(particles.SetPosition(0, {0.0, 0.0, 0.0}) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(particles.SetPosition(1, {1.0, 0.0, 0.0}) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(particles.SetMass(0, 1.0) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(particles.SetMass(1, 1.0) == BLITZAR_STATUS_OK);

    BLITZAR_CHECK(
        particles.SetPosition(2, {0.0, 0.0, 0.0}) == BLITZAR_STATUS_INVALID_ARGUMENT);

    BLITZAR_CHECK(
        particles.SetVelocity(2, {0.0, 0.0, 0.0}) == BLITZAR_STATUS_INVALID_ARGUMENT);

    BLITZAR_CHECK(particles.SetMass(2, 1.0) == BLITZAR_STATUS_INVALID_ARGUMENT);
    BLITZAR_CHECK(particles.SetVelocity(
                      0, {std::numeric_limits<double>::infinity(), 0.0, 0.0}) ==
                  BLITZAR_STATUS_INVALID_ARGUMENT);

    BLITZAR_CHECK(
        particles.SetMass(0, -1.0) == BLITZAR_STATUS_INVALID_ARGUMENT);

    const blitzar_core::ExecutionSettings settings{};
    const blitzar_physics::GravityParameters gravity{1.0, 0.0};

    {
        blitzar_particles::ParticleBuffer singular_particles(3);
        blitzar_particles::AccelerationBuffer singular_accelerations(3);

        BLITZAR_CHECK(singular_particles.SetPosition(0, {0.0, 0.0, 0.0}) == BLITZAR_STATUS_OK);
        BLITZAR_CHECK(singular_particles.SetPosition(1, {1.0, 0.0, 0.0}) == BLITZAR_STATUS_OK);
        BLITZAR_CHECK(singular_particles.SetPosition(2, {1.0, 0.0, 0.0}) == BLITZAR_STATUS_OK);

        for (std::size_t index = 0; index < 3; ++index) {
            BLITZAR_CHECK(singular_particles.SetMass(index, 1.0) == BLITZAR_STATUS_OK);
        }

        const blitzar_core::ForceView force = singular_accelerations.View();

        for (std::size_t index = 0; index < 3; ++index) {
            force.x[index] = 7.0;
            force.y[index] = 8.0;
            force.z[index] = 9.0;
        }

        blitzar_direct::DirectSolver singular_solver(gravity);

        BLITZAR_CHECK(singular_solver.Prepare(3) == BLITZAR_STATUS_OK);
        BLITZAR_CHECK(singular_solver.Compute(singular_particles.State(), force, settings) ==
                      BLITZAR_STATUS_SINGULARITY);

        for (std::size_t index = 0; index < 3; ++index) {
            BLITZAR_CHECK(force.x[index] == 7.0);
            BLITZAR_CHECK(force.y[index] == 8.0);
            BLITZAR_CHECK(force.z[index] == 9.0);
        }
    }

    {
        blitzar_particles::ParticleBuffer large_particles(4);
        blitzar_particles::AccelerationBuffer large_accelerations(4);
        const blitzar_core::Vector3 positions[] = {
            {0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {1.2, 0.0, 0.0}, {1.4, 0.0, 0.0}};

        for (std::size_t index = 0; index < 4; ++index) {
            BLITZAR_CHECK(
                large_particles.SetPosition(index, positions[index]) == BLITZAR_STATUS_OK);

            BLITZAR_CHECK(large_particles.SetMass(index, std::numeric_limits<double>::max()) ==
                          BLITZAR_STATUS_OK);
        }

        const blitzar_core::ForceView force = large_accelerations.View();

        for (std::size_t index = 0; index < 4; ++index) {
            force.x[index] = -11.0;
            force.y[index] = -12.0;
            force.z[index] = -13.0;
        }

        blitzar_direct::DirectSolver large_solver(gravity);

        BLITZAR_CHECK(large_solver.Prepare(4) == BLITZAR_STATUS_OK);
        BLITZAR_CHECK(large_solver.Compute(large_particles.State(), force, settings) ==
                      BLITZAR_STATUS_INVALID_ARGUMENT);

        for (std::size_t index = 0; index < 4; ++index) {
            BLITZAR_CHECK(force.x[index] == -11.0);
            BLITZAR_CHECK(force.y[index] == -12.0);
            BLITZAR_CHECK(force.z[index] == -13.0);
        }
    }

    blitzar_direct::DirectSolver solver(gravity);

    BLITZAR_CHECK(solver.Prepare(2) == BLITZAR_STATUS_OK);

    blitzar_direct::DirectSolver undersized_solver(gravity);

    BLITZAR_CHECK(undersized_solver.Prepare(1) == BLITZAR_STATUS_OK);

    const blitzar_core::ForceView untouched_force = accelerations.View();

    untouched_force.x[0] = 17.0;
    untouched_force.y[0] = 18.0;
    untouched_force.z[0] = 19.0;
    untouched_force.x[1] = 20.0;
    untouched_force.y[1] = 21.0;
    untouched_force.z[1] = 22.0;

    BLITZAR_CHECK(undersized_solver.Compute(particles.State(), untouched_force, settings) ==
                  BLITZAR_STATUS_INVALID_ARGUMENT);

    BLITZAR_CHECK(untouched_force.x[0] == 17.0);
    BLITZAR_CHECK(untouched_force.y[0] == 18.0);
    BLITZAR_CHECK(untouched_force.z[0] == 19.0);
    BLITZAR_CHECK(untouched_force.x[1] == 20.0);
    BLITZAR_CHECK(untouched_force.y[1] == 21.0);
    BLITZAR_CHECK(untouched_force.z[1] == 22.0);
    BLITZAR_CHECK(
        solver.Compute(particles.State(), accelerations.View(), settings) == BLITZAR_STATUS_OK);

    const blitzar_core::ForceView force = accelerations.View();

    BLITZAR_CHECK(std::abs(force.x[0] - 1.0) < 1.0e-12);
    BLITZAR_CHECK(std::abs(force.x[1] + 1.0) < 1.0e-12);
    BLITZAR_CHECK(std::abs(force.y[0]) < 1.0e-12);
    BLITZAR_CHECK(std::abs(force.z[1]) < 1.0e-12);
    BLITZAR_CHECK(particles.SetPosition(1, {0.0, 0.0, 0.0}) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(solver.Compute(particles.State(), accelerations.View(), settings) ==
                  BLITZAR_STATUS_SINGULARITY);

    BLITZAR_CHECK(particles.SetPosition(1, {1.0, 0.0, 0.0}) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(particles.SetMass(1, std::numeric_limits<double>::quiet_NaN()) ==
                  BLITZAR_STATUS_INVALID_ARGUMENT);

    BLITZAR_CHECK(
        solver.Compute(particles.State(), accelerations.View(), settings) == BLITZAR_STATUS_OK);

    BLITZAR_CHECK(particles.SetMass(1, 1.0) == BLITZAR_STATUS_OK);

    blitzar_particles::ParticleBuffer free_particle(1);
    blitzar_particles::AccelerationBuffer free_acceleration(1);
    blitzar_integration::KdkCheckpoint free_checkpoint(1);

    BLITZAR_CHECK(free_particle.SetVelocity(0, {2.0, 0.0, 0.0}) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(free_particle.SetPosition(1, {0.0, 0.0, 0.0}) == BLITZAR_STATUS_INVALID_ARGUMENT);

    const blitzar_integration::LeapfrogKdk integrator{};
    std::span<std::size_t> solver_scratch{};

    blitzar_integration_kdk::AdvanceState free_state{
        free_particle, free_acceleration, free_checkpoint, solver, 0.5, settings, solver_scratch,
        free_particle.State()};

    BLITZAR_CHECK(integrator.Advance(free_state) == BLITZAR_STATUS_OK);

    const blitzar_core::ParticleStateView state = free_particle.State();

    BLITZAR_CHECK(std::abs(state.x[0] - 1.0) < 1.0e-12);
    BLITZAR_CHECK(std::abs(state.velocity_x[0] - 2.0) < 1.0e-12);

    free_state.timestep = 0.0;

    BLITZAR_CHECK(integrator.Advance(free_state) == BLITZAR_STATUS_INVALID_ARGUMENT);

    blitzar_particles::ParticleBuffer rollback_particle(1);
    blitzar_particles::AccelerationBuffer rollback_acceleration(1);
    blitzar_integration::KdkCheckpoint rollback_checkpoint(1);

    BLITZAR_CHECK(rollback_particle.SetVelocity(0, {1.0, 0.0, 0.0}) == BLITZAR_STATUS_OK);

    FailOnSecondSolver failing_solver{};
    blitzar_integration_kdk::AdvanceState rollback_state{
        rollback_particle, rollback_acceleration, rollback_checkpoint, failing_solver, 0.5,
        settings, solver_scratch, rollback_particle.State()};

    BLITZAR_CHECK(integrator.Advance(rollback_state) == BLITZAR_STATUS_INTERNAL_ERROR);

    const blitzar_core::ParticleStateView restored = rollback_particle.State();

    BLITZAR_CHECK(std::abs(restored.x[0]) < 1.0e-12);
    BLITZAR_CHECK(std::abs(restored.velocity_x[0] - 1.0) < 1.0e-12);

    blitzar_particles::ParticleBuffer external_rollback_particle(1);
    blitzar_particles::AccelerationBuffer external_rollback_acceleration(1);
    blitzar_integration::KdkCheckpoint external_rollback_checkpoint(1);

    BLITZAR_CHECK(external_rollback_particle.SetVelocity(0, {1.0, 0.0, 0.0}) == BLITZAR_STATUS_OK);

    bool drift_mutated_state = false;
    std::size_t rollback_calls = 0;
    auto mutating_drift =
        [&drift_mutated_state](blitzar_particles::ParticleBuffer& current_particles,
            blitzar_particles::AccelerationBuffer&,
            blitzar_integration::KdkCheckpoint&) -> blitzar_integration_kdk::DriftTransition {
        drift_mutated_state = true;

        const blitzar_status status = current_particles.SetCount(0);

        return {status, status == BLITZAR_STATUS_OK};
    };

    auto restore_external_state = [&]() noexcept {
        ++rollback_calls;

        drift_mutated_state = false;

        (void)external_rollback_particle.SetCount(1);
        (void)external_rollback_acceleration.SetCount(1);
        (void)external_rollback_checkpoint.SetCount(1);
    };

    FailOnSecondSolver external_failing_solver{};
    std::span<std::size_t> external_solver_scratch{};

    blitzar_integration_kdk::AdvanceState external_state{
        external_rollback_particle, external_rollback_acceleration, external_rollback_checkpoint,
        external_failing_solver, 0.5, settings, external_solver_scratch,
        external_rollback_particle.State()};

    blitzar_integration_kdk::AdvanceHooks external_hooks{mutating_drift, restore_external_state};
    blitzar_integration_kdk::AdvanceRequest external_request{external_state, external_hooks};

    BLITZAR_CHECK(integrator.Advance(external_request) == BLITZAR_STATUS_INVALID_ARGUMENT);
    BLITZAR_CHECK(rollback_calls == 1);
    BLITZAR_CHECK(!drift_mutated_state);
    BLITZAR_CHECK(external_rollback_particle.Count() == 1);
    BLITZAR_CHECK(external_rollback_acceleration.Count() == 1);
    BLITZAR_CHECK(external_rollback_checkpoint.Count() == 1);

    const blitzar_core::ParticleStateView externally_restored = external_rollback_particle.State();

    BLITZAR_CHECK(std::abs(externally_restored.x[0]) < 1.0e-12);
    BLITZAR_CHECK(std::abs(externally_restored.velocity_x[0] - 1.0) < 1.0e-12);

    blitzar_particles::ParticleBuffer non_finite_particle(1);
    blitzar_particles::AccelerationBuffer non_finite_acceleration(1);
    blitzar_integration::KdkCheckpoint non_finite_checkpoint(1);
    NonFiniteSolver non_finite_solver{};
    blitzar_integration_kdk::AdvanceState non_finite_state{
        non_finite_particle, non_finite_acceleration, non_finite_checkpoint, non_finite_solver,
        0.5, settings, solver_scratch, non_finite_particle.State()};

    BLITZAR_CHECK(non_finite_particle.SetVelocity(0, {1.0, 0.0, 0.0}) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(integrator.Advance(non_finite_state) == BLITZAR_STATUS_INVALID_ARGUMENT);

    const blitzar_core::ParticleStateView finite_state = non_finite_particle.State();

    BLITZAR_CHECK(finite_state.x[0] == 0.0);
    BLITZAR_CHECK(finite_state.velocity_x[0] == 1.0);

    return 0;
}
