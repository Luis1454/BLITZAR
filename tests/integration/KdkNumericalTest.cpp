#include "core/CoreExecution.hpp"
#include "fixtures/FixtureCheck.hpp"
#include "integration/kdk/KdkLeapfrog.hpp"
#include "particles/buffer/ParticleAccelerationBuffer.hpp"
#include "particles/buffer/ParticleBuffer.hpp"
#include "solvers/direct/DirectSolver.hpp"

#include <cmath>
#include <cstddef>
#include <limits>
#include <span>

namespace {

struct Vector final {
    double x{};
    double y{};
    double z{};
};

bool Initialize(blitzar_particles::ParticleBuffer& particles) noexcept
{
    const double softening = 0.05;
    const double circular_speed = std::sqrt(0.5 / std::pow(1.0 + softening * softening, 1.5));

    return particles.SetPosition(0, {-0.5, 0.0, 0.0}) == BLITZAR_STATUS_OK &&
           particles.SetPosition(1, {0.5, 0.0, 0.0}) == BLITZAR_STATUS_OK &&
           particles.SetVelocity(0, {0.0, circular_speed, 0.0}) == BLITZAR_STATUS_OK &&
           particles.SetVelocity(1, {0.0, -circular_speed, 0.0}) == BLITZAR_STATUS_OK &&
           particles.SetMass(0, 1.0) == BLITZAR_STATUS_OK &&
           particles.SetMass(1, 1.0) == BLITZAR_STATUS_OK;
}

Vector Momentum(blitzar_core::ParticleStateView state) noexcept
{
    Vector momentum{};

    for (std::size_t index = 0; index < state.count; ++index) {
        momentum.x += state.mass[index] * state.velocity_x[index];
        momentum.y += state.mass[index] * state.velocity_y[index];
        momentum.z += state.mass[index] * state.velocity_z[index];
    }

    return momentum;
}

double Energy(
    blitzar_core::ParticleStateView state, double gravitational_constant, double softening) noexcept
{
    double kinetic = 0.0;

    for (std::size_t index = 0; index < state.count; ++index) {
        const double speed_squared = state.velocity_x[index] * state.velocity_x[index] +
                                     state.velocity_y[index] * state.velocity_y[index] +
                                     state.velocity_z[index] * state.velocity_z[index];

        kinetic += 0.5 * state.mass[index] * speed_squared;
    }

    const double dx = state.x[1] - state.x[0];
    const double dy = state.y[1] - state.y[0];
    const double dz = state.z[1] - state.z[0];
    const double softened_distance = std::sqrt(dx * dx + dy * dy + dz * dz + softening * softening);
    const double potential =
        -gravitational_constant * state.mass[0] * state.mass[1] / softened_distance;

    return kinetic + potential;
}

bool SameState(
    blitzar_core::ParticleStateView first, blitzar_core::ParticleStateView second) noexcept
{
    for (std::size_t index = 0; index < first.count; ++index) {
        if (first.x[index] != second.x[index] || first.y[index] != second.y[index] ||
            first.z[index] != second.z[index] ||
            first.velocity_x[index] != second.velocity_x[index] ||
            first.velocity_y[index] != second.velocity_y[index] ||
            first.velocity_z[index] != second.velocity_z[index] ||
            first.mass[index] != second.mass[index]) {
            return false;
        }
    }

    return true;
}

} // namespace

int main()
{
    constexpr double gravitational_constant = 1.0;
    constexpr double softening = 0.05;
    constexpr double timestep = 0.001;
    constexpr std::size_t steps = 4096;
    blitzar_particles::ParticleBuffer first_particles(2);
    blitzar_particles::ParticleBuffer second_particles(2);

    BLITZAR_CHECK(Initialize(first_particles));
    BLITZAR_CHECK(Initialize(second_particles));

    blitzar_particles::ParticleAccelerationBuffer first_accelerations(2);
    blitzar_particles::ParticleAccelerationBuffer second_accelerations(2);
    blitzar_integration::KdkCheckpoint first_checkpoint(2);
    blitzar_integration::KdkCheckpoint second_checkpoint(2);
    blitzar_direct::DirectSolver solver({gravitational_constant, softening});

    BLITZAR_CHECK(solver.Prepare(2) == BLITZAR_STATUS_OK);

    const blitzar_core::ExecutionSettings settings{};
    const blitzar_integration::KdkLeapfrog integrator{};
    std::span<std::size_t> solver_scratch{};

    blitzar_integration_kdk::AdvanceState<blitzar_direct::DirectSolver, std::span<std::size_t>>
        first_state_request{first_particles, first_accelerations, first_checkpoint, solver,
            timestep, settings, solver_scratch, first_particles.State()};

    blitzar_integration_kdk::AdvanceState<blitzar_direct::DirectSolver, std::span<std::size_t>>
        second_state_request{second_particles, second_accelerations, second_checkpoint, solver,
            timestep, settings, solver_scratch, second_particles.State()};

    const double initial_energy =
        Energy(first_particles.State(), gravitational_constant, softening);

    for (std::size_t step = 0; step < steps; ++step) {
        BLITZAR_CHECK(integrator.Advance(first_state_request) == BLITZAR_STATUS_OK);
        BLITZAR_CHECK(integrator.Advance(second_state_request) == BLITZAR_STATUS_OK);
    }

    const blitzar_core::ParticleStateView first_state = first_particles.State();
    const blitzar_core::ParticleStateView second_state = second_particles.State();
    const Vector momentum = Momentum(first_state);

    BLITZAR_CHECK(std::abs(momentum.x) < 1.0e-12);
    BLITZAR_CHECK(std::abs(momentum.y) < 1.0e-12);
    BLITZAR_CHECK(std::abs(momentum.z) < 1.0e-12);
    BLITZAR_CHECK(
        std::abs(Energy(first_state, gravitational_constant, softening) - initial_energy) < 1.0e-8);

    BLITZAR_CHECK(SameState(first_state, second_state));

    blitzar_particles::ParticleBuffer limit_particles(1);
    blitzar_particles::ParticleAccelerationBuffer limit_accelerations(1);
    blitzar_integration::KdkCheckpoint limit_checkpoint(1);

    blitzar_integration_kdk::AdvanceState<blitzar_direct::DirectSolver, std::span<std::size_t>>
        limit_state{limit_particles, limit_accelerations, limit_checkpoint, solver,
            std::numeric_limits<double>::infinity(), settings, solver_scratch,
            limit_particles.State()};

    BLITZAR_CHECK(
        limit_particles.SetMass(0, std::numeric_limits<double>::max()) == BLITZAR_STATUS_OK);

    BLITZAR_CHECK(integrator.Advance(limit_state) == BLITZAR_STATUS_INVALID_ARGUMENT);

    return 0;
}
