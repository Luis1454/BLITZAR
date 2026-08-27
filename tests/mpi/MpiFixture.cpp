#include "MpiCases.hpp"
#include "fixtures/FixtureViews.hpp"
#include "integration/kdk/KdkLeapfrog.hpp"
#include "particles/buffer/ParticleAccelerationBuffer.hpp"
#include "particles/buffer/ParticleBuffer.hpp"
#include "solvers/direct/DirectSolver.hpp"

#include <cmath>
#include <cstddef>
#include <span>

#if defined(BLITZAR_HAS_MPI)
#include <mpi.h>
#endif

namespace blitzar_mpi_tests {

namespace {

bool AdvanceSimulation(blitzar_sim::Sim& simulation, int step_count) noexcept
{
    for (int step = 0; step < step_count; ++step) {
        if (simulation.Step() != BLITZAR_STATUS_OK) {
            return false;
        }
    }

    return true;
}

bool StatesNear(const StateArrays& left, const StateArrays& right, double tolerance) noexcept
{
    for (std::size_t index = 0; index < ParticleCount; ++index) {
        if (std::abs(left.x[index] - right.x[index]) >= tolerance ||
            std::abs(left.y[index] - right.y[index]) >= tolerance ||
            std::abs(left.z[index] - right.z[index]) >= tolerance ||
            std::abs(left.velocity_x[index] - right.velocity_x[index]) >= tolerance ||
            std::abs(left.velocity_y[index] - right.velocity_y[index]) >= tolerance ||
            std::abs(left.velocity_z[index] - right.velocity_z[index]) >= tolerance ||
            left.mass[index] != right.mass[index]) {
            return false;
        }
    }

    return true;
}

bool ValidateRejectedInput(
    blitzar_sim::Sim& simulation, const StateArrays& initial, const StateArrays& reference) noexcept
{
    StateArrays rejected = initial;

    rejected.x[0] += 100.0;
    rejected.mass[0] = -1.0;

    if (simulation.SetParticles(blitzar_tests::MakeStateView(rejected)) !=
        BLITZAR_STATUS_INVALID_ARGUMENT) {
        return false;
    }

    StateArrays after_rejected{};

    return simulation.GetState(blitzar_tests::MakeOutputView(after_rejected)) ==
               BLITZAR_STATUS_OK &&
           StatesNear(after_rejected, reference, 1.0e-5);
}

} // namespace

StateArrays InitialState() noexcept
{
    StateArrays state{};

    for (std::size_t index = 0; index < ParticleCount; ++index) {
        const double value = static_cast<double>(index);

        state.x[index] = -3.5 + value;
        state.y[index] = (index % 2 == 0 ? -1.0 : 1.0) + 0.1 * value;
        state.z[index] = (index % 3 == 0 ? 1.0 : -1.0) - 0.05 * value;
        state.velocity_x[index] = 0.02 * (3.5 - value);
        state.velocity_y[index] = -0.01 * value;
        state.velocity_z[index] = 0.015 * value;
        state.mass[index] = 1.0 + 0.25 * static_cast<double>(index % 3);
    }

    return state;
}

StateArrays MigrationState() noexcept
{
    StateArrays state = InitialState();

    for (std::size_t index = 0; index < ParticleCount; ++index) {
        state.velocity_x[index] = index % 2 == 0 ? 50.0 : -50.0;
        state.velocity_y[index] = 0.0;
        state.velocity_z[index] = 0.0;
    }

    return state;
}

bool Configure(blitzar_sim::Sim& simulation, const StateArrays& state, double timestep,
    blitzar_solver_kind solver_kind) noexcept
{
    if (solver_kind == BLITZAR_SOLVER_BARNES_HUT &&
        simulation.SetBarnesHut({0.0, ParticleCount, 128, 1, 32}) != BLITZAR_STATUS_OK) {
        return false;
    }

    blitzar_core::ParticleStateView input = blitzar_tests::MakeStateView(state);

#if defined(BLITZAR_HAS_MPI)
    int rank = 0;

    if (MPI_Comm_rank(MPI_COMM_WORLD, &rank) != MPI_SUCCESS) {
        return false;
    }

    if (rank != 0) {
        input = {};
    }

#endif

    return simulation.SetSolver(solver_kind) == BLITZAR_STATUS_OK &&
           simulation.SetGravity(1.0, 0.1) == BLITZAR_STATUS_OK &&
           simulation.SetTimestep(timestep) == BLITZAR_STATUS_OK &&
           simulation.SetParticles(input) == BLITZAR_STATUS_OK;
}

bool BuildReference(
    const StateArrays& initial, StateArrays& result, double timestep, int step_count) noexcept
{
    blitzar_particles::ParticleBuffer particles(ParticleCount);
    blitzar_particles::ParticleAccelerationBuffer accelerations(ParticleCount);
    blitzar_integration::KdkCheckpoint checkpoint(ParticleCount);

    for (std::size_t index = 0; index < ParticleCount; ++index) {
        if (particles.SetPosition(index, {initial.x[index], initial.y[index], initial.z[index]}) !=

                BLITZAR_STATUS_OK ||
            particles.SetVelocity(index, {initial.velocity_x[index], initial.velocity_y[index],
                                             initial.velocity_z[index]}) != BLITZAR_STATUS_OK ||
            particles.SetMass(index, initial.mass[index]) != BLITZAR_STATUS_OK) {
            return false;
        }
    }

    const blitzar_physics::GravityParameters gravity{1.0, 0.1};
    blitzar_direct::DirectSolver solver(gravity);

    if (solver.Prepare(ParticleCount) != BLITZAR_STATUS_OK) {
        return false;
    }

    const blitzar_core::ExecutionSettings execution{};
    const blitzar_integration::KdkLeapfrog integrator{};
    std::span<std::size_t> solver_scratch{};

    for (int step = 0; step < step_count; ++step) {
        blitzar_integration_kdk::AdvanceState state{particles, accelerations, checkpoint, solver,
            timestep, execution, solver_scratch, particles.State()};

        if (integrator.Advance(state) != BLITZAR_STATUS_OK) {
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

bool RunCase(const StateArrays& initial, double timestep, int step_count,
    blitzar_solver_kind solver_kind) noexcept
{
    StateArrays reference{};

    if (!BuildReference(initial, reference, timestep, step_count)) {
        return false;
    }

    blitzar_sim::Sim simulation(ParticleCount);

    if (!Configure(simulation, initial, timestep, solver_kind) ||
        !AdvanceSimulation(simulation, step_count)) {
        return false;
    }

    StateArrays distributed{};

    if (simulation.GetState(blitzar_tests::MakeOutputView(distributed)) != BLITZAR_STATUS_OK ||
        !StatesNear(distributed, reference, 1.0e-5)) {
        return false;
    }

    return ValidateRejectedInput(simulation, initial, reference);
}

bool StatesMatch(const StateArrays& left, const StateArrays& right) noexcept
{
    for (std::size_t index = 0; index < ParticleCount; ++index) {
        if (std::abs(left.x[index] - right.x[index]) >= 1.0e-10 ||
            std::abs(left.y[index] - right.y[index]) >= 1.0e-10 ||
            std::abs(left.z[index] - right.z[index]) >= 1.0e-10 ||
            std::abs(left.velocity_x[index] - right.velocity_x[index]) >= 1.0e-10 ||
            std::abs(left.velocity_y[index] - right.velocity_y[index]) >= 1.0e-10 ||
            std::abs(left.velocity_z[index] - right.velocity_z[index]) >= 1.0e-10 ||
            left.mass[index] != right.mass[index]) {
            return false;
        }
    }

    return true;
}

} // namespace blitzar_mpi_tests
