#include "integration/LeapfrogKdk.hpp"
#include "parallel/MpiContext.hpp"
#include "particles/ParticleBuffer.hpp"
#include "sdk/Simulation.hpp"
#include "solvers/direct/DirectSolver.hpp"

#include "Check.hpp"

#include <mpi.h>

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

[[nodiscard]] bool Configure(
    blitzar_sdk::Simulation& simulation,
    const StateArrays& state) noexcept
{
    return simulation.SetSolver(BLITZAR_SOLVER_DIRECT) == BLITZAR_STATUS_OK &&
           simulation.SetGravity(1.0, 0.1) == BLITZAR_STATUS_OK &&
           simulation.SetTimestep(0.01) == BLITZAR_STATUS_OK &&
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
    StateArrays& result) noexcept
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
    for (int step = 0; step < 2; ++step) {
        if (integrator.Advance(
                particles,
                accelerations,
                workspace,
                solver,
                0.01,
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

}  // namespace

int main()
{
    blitzar_parallel::MpiContext context;
    bool local_ok = context.IsUsable() && (context.Size() == 2 || context.Size() == 4);
    const StateArrays initial = InitialState();
    StateArrays reference{};
    const bool reference_ok = BuildReference(initial, reference);
    local_ok = local_ok && reference_ok;

    blitzar_sdk::Simulation simulation(ParticleCount);
    const bool configuration_ok = Configure(simulation, initial);
    local_ok = local_ok && configuration_ok;
    for (int step = 0; step < 2 && local_ok; ++step) {
        const blitzar_status step_status = simulation.Step();
        local_ok = step_status == BLITZAR_STATUS_OK;
    }

    StateArrays distributed{};
    if (local_ok) {
        local_ok = simulation.GetState(
                       distributed.x,
                       distributed.y,
                       distributed.z,
                       distributed.velocity_x,
                       distributed.velocity_y,
                       distributed.velocity_z,
                       distributed.mass) == BLITZAR_STATUS_OK;
    }
    if (local_ok) {
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
    }

    int local_failure = local_ok ? 0 : 1;
    int global_failure = 0;
    BLITZAR_CHECK(
        MPI_Allreduce(
            &local_failure,
            &global_failure,
            1,
            MPI_INT,
            MPI_MAX,
            context.Communicator()) == MPI_SUCCESS);
    BLITZAR_CHECK(global_failure == 0);
    return 0;
}
