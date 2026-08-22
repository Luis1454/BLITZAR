#include "integration/LeapfrogKdk.hpp"
#include "parallel/MpiContext.hpp"
#include "particles/ParticleBuffer.hpp"
#include "sdk/Simulation.hpp"
#include "solvers/direct/DirectSolver.hpp"

#include "Check.hpp"

#include <array>
#include <cmath>
#include <cstddef>

namespace {

constexpr std::size_t ParticleCount = 8;

using StateArray = std::array<double, ParticleCount>;

struct StateArrays final {
    StateArray x{};
    StateArray y{};
    StateArray z{};
    StateArray velocity_x{};
    StateArray velocity_y{};
    StateArray velocity_z{};
    StateArray mass{};
};

[[nodiscard]] StateArrays InitialState() noexcept
{
    StateArrays state{};
    for (std::size_t index = 0; index < ParticleCount; ++index) {
        const double value = static_cast<double>(index);
        state.x[index] = -3.5 + value;
        state.y[index] = (index % 2 == 0 ? -1.0 : 1.0) + 0.1 * value;
        state.z[index] = (index % 3 == 0 ? 1.0 : -1.0) - 0.05 * value;
        state.velocity_x[index] = 0.02 * value;
        state.velocity_y[index] = -0.01 * value;
        state.velocity_z[index] = 0.015 * value;
        state.mass[index] = 1.0 + 0.25 * static_cast<double>(index % 3);
    }
    return state;
}

[[nodiscard]] StateArrays MigrationState() noexcept
{
    StateArrays state = InitialState();
    for (std::size_t index = 0; index < ParticleCount; ++index) {
        state.velocity_x[index] = index % 2 == 0 ? 500.0 : -500.0;
        state.velocity_y[index] = 0.0;
        state.velocity_z[index] = 0.0;
    }
    return state;
}

[[nodiscard]] bool Configure(
    blitzar_sdk::Simulation& simulation,
    const StateArrays& state,
    double timestep) noexcept
{
    return simulation.SetSolver(BLITZAR_SOLVER_DIRECT) == BLITZAR_STATUS_OK &&
           simulation.SetGravity(1.0, 0.1) == BLITZAR_STATUS_OK &&
           simulation.SetTimestep(timestep) == BLITZAR_STATUS_OK &&
           simulation.SetParticles(
               state.x,
               state.y,
               state.z,
               state.velocity_x,
               state.velocity_y,
               state.velocity_z,
               state.mass) == BLITZAR_STATUS_OK;
}

[[nodiscard]] bool BuildReference(
    const StateArrays& initial,
    StateArrays& result,
    double timestep,
    int step_count) noexcept
{
    blitzar_particles::ParticleBuffer particles(ParticleCount);
    blitzar_particles::AccelerationBuffer accelerations(ParticleCount);
    blitzar_integration::LeapfrogWorkspace workspace(ParticleCount);
    for (std::size_t index = 0; index < ParticleCount; ++index) {
        if (particles.SetPosition(
                index,
                {initial.x[index], initial.y[index], initial.z[index]}) !=
                BLITZAR_STATUS_OK ||
            particles.SetVelocity(
                index,
                {initial.velocity_x[index],
                 initial.velocity_y[index],
                 initial.velocity_z[index]}) != BLITZAR_STATUS_OK ||
            particles.SetMass(index, initial.mass[index]) != BLITZAR_STATUS_OK) {
            return false;
        }
    }
    const blitzar_physics::GravityParameters gravity{1.0, 0.1};
    blitzar_direct::DirectSolver solver(gravity);
    const blitzar_core::ExecutionSettings execution{};
    const blitzar_integration::LeapfrogKdk integrator{};
    for (int step = 0; step < step_count; ++step) {
        if (integrator.Advance(
                particles,
                accelerations,
                workspace,
                solver,
                timestep,
                execution) != BLITZAR_STATUS_OK) {
            return false;
        }
    }
    const blitzar_core::ParticleStateView state = particles.State();
    for (std::size_t index = 0; index < ParticleCount; ++index) {
        result.x[index] = state.x[index];
        result.y[index] = state.y[index];
        result.z[index] = state.z[index];
        result.velocity_x[index] = state.velocity_x[index];
        result.velocity_y[index] = state.velocity_y[index];
        result.velocity_z[index] = state.velocity_z[index];
        result.mass[index] = state.mass[index];
    }
    return true;
}

[[nodiscard]] bool RunCase(
    const StateArrays& initial,
    double timestep,
    int step_count) noexcept
{
    StateArrays reference{};
    bool local_ok = BuildReference(
        initial,
        reference,
        timestep,
        step_count);

    blitzar_sdk::Simulation simulation(ParticleCount);
    const bool configuration_ok = Configure(simulation, initial, timestep);
    local_ok = local_ok && configuration_ok;
    for (int step = 0; step < step_count; ++step) {
        const blitzar_status step_status = simulation.Step();
        local_ok = local_ok && step_status == BLITZAR_STATUS_OK;
    }

    StateArrays distributed{};
    const blitzar_status state_status = simulation.GetState(
        distributed.x,
        distributed.y,
        distributed.z,
        distributed.velocity_x,
        distributed.velocity_y,
        distributed.velocity_z,
        distributed.mass);
    local_ok = local_ok && state_status == BLITZAR_STATUS_OK;
    for (std::size_t index = 0; index < ParticleCount; ++index) {
        local_ok = local_ok &&
                   std::abs(distributed.x[index] - reference.x[index]) < 1.0e-5 &&
                   std::abs(distributed.y[index] - reference.y[index]) < 1.0e-5 &&
                   std::abs(distributed.z[index] - reference.z[index]) < 1.0e-5 &&
                   std::abs(distributed.velocity_x[index] -
                            reference.velocity_x[index]) < 1.0e-5 &&
                   std::abs(distributed.velocity_y[index] -
                            reference.velocity_y[index]) < 1.0e-5 &&
                   std::abs(distributed.velocity_z[index] -
                            reference.velocity_z[index]) < 1.0e-5 &&
                   distributed.mass[index] == reference.mass[index];
    }

    StateArrays rejected = initial;
    rejected.x[0] += 100.0;
    rejected.mass[0] = -1.0;
    local_ok = local_ok &&
               simulation.SetParticles(
                   rejected.x,
                   rejected.y,
                   rejected.z,
                   rejected.velocity_x,
                   rejected.velocity_y,
                   rejected.velocity_z,
                   rejected.mass) == BLITZAR_STATUS_INVALID_ARGUMENT;

    StateArrays after_rejected{};
    const blitzar_status rejected_state_status = simulation.GetState(
        after_rejected.x,
        after_rejected.y,
        after_rejected.z,
        after_rejected.velocity_x,
        after_rejected.velocity_y,
        after_rejected.velocity_z,
        after_rejected.mass);
    local_ok = local_ok && rejected_state_status == BLITZAR_STATUS_OK;
    for (std::size_t index = 0; index < ParticleCount; ++index) {
        local_ok = local_ok &&
                   std::abs(after_rejected.x[index] - reference.x[index]) <
                       1.0e-5 &&
                   std::abs(after_rejected.y[index] - reference.y[index]) <
                       1.0e-5 &&
                   std::abs(after_rejected.z[index] - reference.z[index]) <
                       1.0e-5 &&
                   std::abs(after_rejected.velocity_x[index] -
                            reference.velocity_x[index]) <
                       1.0e-5 &&
                   std::abs(after_rejected.velocity_y[index] -
                            reference.velocity_y[index]) <
                       1.0e-5 &&
                   std::abs(after_rejected.velocity_z[index] -
                            reference.velocity_z[index]) <
                       1.0e-5 &&
                   after_rejected.mass[index] == reference.mass[index];
    }
    return local_ok;
}

}  // namespace

int main(int argc, char** argv)
{
    (void)argv;
    blitzar_parallel::MpiContext context;
    const bool valid_world =
        context.IsUsable() && (context.Size() == 2 || context.Size() == 4);
    const bool migration_case = argc > 1;
    const bool local_case = RunCase(
        migration_case ? MigrationState() : InitialState(),
        0.01,
        migration_case ? 1 : 2);
    const bool local_ok = valid_world && local_case;

    int local_failure = local_ok ? 0 : 1;
    int global_failure = 0;
    BLITZAR_CHECK(
        context.ReduceMax(local_failure, global_failure) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(global_failure == 0);
    return 0;
}
