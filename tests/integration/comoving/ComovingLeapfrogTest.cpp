#include "integration/comoving/ComovingLeapfrog.hpp"

#include "core/CoreExecution.hpp"
#include "fixtures/FixtureCheck.hpp"
#include "integration/comoving/ComovingBackground.hpp"
#include "integration/kdk/KdkLeapfrog.hpp"
#include "particles/buffer/ParticleAccelerationBuffer.hpp"
#include "particles/buffer/ParticleBuffer.hpp"
#include "solvers/SolverCpuForceProvider.hpp"
#include "solvers/direct/DirectSolver.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>

namespace {

bool ExactlySame(
    blitzar_core::ParticleStateView first, blitzar_core::ParticleStateView second) noexcept
{
    if (first.count != second.count) {
        return false;
    }

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

bool SeedCircularPair(blitzar_particles::ParticleBuffer& particles, double softening) noexcept
{
    const double circular_speed = std::sqrt(0.5 / std::pow(1.0 + softening * softening, 1.5));

    return particles.SetPosition(0, {-0.5, 0.0, 0.0}) == BLITZAR_STATUS_OK &&
           particles.SetPosition(1, {0.5, 0.0, 0.0}) == BLITZAR_STATUS_OK &&
           particles.SetVelocity(0, {0.0, circular_speed, 0.0}) == BLITZAR_STATUS_OK &&
           particles.SetVelocity(1, {0.0, -circular_speed, 0.0}) == BLITZAR_STATUS_OK &&
           particles.SetMass(0, 1.0) == BLITZAR_STATUS_OK &&
           particles.SetMass(1, 1.0) == BLITZAR_STATUS_OK;
}

bool SeedDriftingPair(blitzar_particles::ParticleBuffer& particles) noexcept
{
    return particles.SetPosition(0, {0.5, 0.0, 0.0}) == BLITZAR_STATUS_OK &&
           particles.SetPosition(1, {-0.5, 0.0, 0.0}) == BLITZAR_STATUS_OK &&
           particles.SetVelocity(0, {0.2, -0.1, 0.05}) == BLITZAR_STATUS_OK &&
           particles.SetVelocity(1, {-0.3, 0.12, 0.0}) == BLITZAR_STATUS_OK &&
           particles.SetMass(0, 0.0) == BLITZAR_STATUS_OK &&
           particles.SetMass(1, 0.0) == BLITZAR_STATUS_OK;
}

class FailOnSecondSolver final {
public:
    [[nodiscard]] blitzar_solvers::SolverKind Kind() const noexcept
    {
        return blitzar_solvers::SolverKind::Direct;
    }

    [[nodiscard]] blitzar_status Evaluate(
        const blitzar_solvers::SolverForceEvaluation& request) noexcept
    {
        if (!blitzar_core::IsValid(request.targets) || !blitzar_core::IsValid(request.forces) ||
            request.targets.count != request.forces.count || !request.settings.IsValid()) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        ++calls_;

        if (calls_ == 2) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }

        for (std::size_t index = 0; index < request.forces.count; ++index) {
            request.forces.x[index] = 0.0;
            request.forces.y[index] = 0.0;
            request.forces.z[index] = 0.0;
        }

        return BLITZAR_STATUS_OK;
    }

private:
    std::size_t calls_{};
};

int CheckStaticParityWithKdk() noexcept
{
    constexpr double timestep = 0.001;
    constexpr double softening = 0.05;
    constexpr std::size_t steps = 4096;
    constexpr double start_time = 1.0;

    const blitzar_integration::ComovingBackground background{};
    const blitzar_core::ExecutionSettings settings{};
    const blitzar_physics::GravityParameters gravity{1.0, softening};
    blitzar_direct::DirectSolver solver(gravity);

    BLITZAR_CHECK(solver.Prepare(2) == BLITZAR_STATUS_OK);

    blitzar_solvers::SolverCpuForceProvider<blitzar_direct::DirectSolver> force_provider(solver);

    blitzar_particles::ParticleBuffer kdk_particles(2);
    blitzar_particles::ParticleAccelerationBuffer kdk_accelerations(2);
    blitzar_integration::KdkCheckpoint kdk_checkpoint(2);
    blitzar_integration::KdkLeapfrog kdk_integrator{};

    BLITZAR_CHECK(SeedCircularPair(kdk_particles, softening));

    blitzar_integration_kdk::AdvanceState<decltype(force_provider)> kdk_state{kdk_particles,
        kdk_accelerations, kdk_checkpoint, force_provider, timestep, settings,
        kdk_particles.State()};

    blitzar_particles::ParticleBuffer comoving_particles(2);
    blitzar_particles::ParticleAccelerationBuffer comoving_accelerations(2);
    blitzar_integration::KdkCheckpoint comoving_checkpoint(2);
    blitzar_integration::ComovingLeapfrog comoving_integrator{};

    BLITZAR_CHECK(SeedCircularPair(comoving_particles, softening));

    blitzar_integration_comoving::AdvanceState<decltype(force_provider)> comoving_state{
        comoving_particles, comoving_accelerations, comoving_checkpoint, force_provider, start_time,
        timestep, background, settings, comoving_particles.State()};

    for (std::size_t step = 0; step < steps; ++step) {
        BLITZAR_CHECK(kdk_integrator.Advance(kdk_state) == BLITZAR_STATUS_OK);
        BLITZAR_CHECK(comoving_integrator.Advance(comoving_state) == BLITZAR_STATUS_OK);
    }

    BLITZAR_CHECK(ExactlySame(kdk_particles.State(), comoving_particles.State()));
    BLITZAR_CHECK(std::abs(comoving_state.cosmic_time - (start_time + steps * timestep)) < 1.0e-9);

    return 0;
}

int CheckEmptyUniverseExpandingOracle() noexcept
{
    constexpr double timestep = 0.001;
    constexpr std::size_t steps = 2000;
    constexpr double start_time = 1.0;

    const blitzar_integration::ComovingBackground background{
        blitzar_integration::ExpansionKind::EinsteinDeSitter, 1.0};

    const blitzar_core::ExecutionSettings settings{};
    const blitzar_physics::GravityParameters gravity{1.0, 0.05};
    blitzar_direct::DirectSolver solver(gravity);

    BLITZAR_CHECK(solver.Prepare(2) == BLITZAR_STATUS_OK);

    blitzar_solvers::SolverCpuForceProvider<blitzar_direct::DirectSolver> force_provider(solver);
    blitzar_particles::ParticleBuffer particles(2);
    blitzar_particles::ParticleAccelerationBuffer accelerations(2);
    blitzar_integration::KdkCheckpoint checkpoint(2);
    blitzar_integration::ComovingLeapfrog integrator{};

    BLITZAR_CHECK(SeedDriftingPair(particles));

    const blitzar_core::ParticleStateView initial = particles.State();
    const double initial_w0 = initial.velocity_x[0];
    const double initial_x0 = initial.x[0];

    blitzar_integration_comoving::AdvanceState<decltype(force_provider)> state{particles,
        accelerations, checkpoint, force_provider, start_time, timestep, background, settings,
        particles.State()};

    for (std::size_t step = 0; step < steps; ++step) {
        BLITZAR_CHECK(integrator.Advance(state) == BLITZAR_STATUS_OK);
    }

    const blitzar_core::ParticleStateView final = particles.State();

    BLITZAR_CHECK(final.velocity_x[0] == initial.velocity_x[0]);
    BLITZAR_CHECK(final.velocity_y[0] == initial.velocity_y[0]);
    BLITZAR_CHECK(final.velocity_z[0] == initial.velocity_z[0]);
    BLITZAR_CHECK(final.velocity_x[1] == initial.velocity_x[1]);
    BLITZAR_CHECK(final.velocity_y[1] == initial.velocity_y[1]);
    BLITZAR_CHECK(final.velocity_z[1] == initial.velocity_z[1]);

    const double end_time = start_time + steps * timestep;
    const double drift = 3.0 * (std::cbrt(end_time) - std::cbrt(start_time));
    const double expected_x = initial_x0 + initial_w0 * drift;

    BLITZAR_CHECK(std::abs(final.x[0] - expected_x) < 1.0e-8);

    return 0;
}

int CheckExpandingTwoBodyConvergence() noexcept
{
    constexpr double softening = 0.05;
    constexpr double start_time = 1.0;
    constexpr double coarse_step = 0.004;
    constexpr std::size_t coarse_count = 256;

    const blitzar_integration::ComovingBackground background{
        blitzar_integration::ExpansionKind::EinsteinDeSitter, 1.0};

    const blitzar_core::ExecutionSettings settings{};
    const blitzar_physics::GravityParameters gravity{1.0, softening};
    blitzar_direct::DirectSolver solver(gravity);

    BLITZAR_CHECK(solver.Prepare(2) == BLITZAR_STATUS_OK);

    blitzar_solvers::SolverCpuForceProvider<blitzar_direct::DirectSolver> force_provider(solver);
    blitzar_integration::ComovingLeapfrog integrator{};

    auto run_fixed = [&](double timestep, std::size_t steps, std::array<double, 6>& out) -> int {
        blitzar_particles::ParticleBuffer particles(2);
        blitzar_particles::ParticleAccelerationBuffer accelerations(2);
        blitzar_integration::KdkCheckpoint checkpoint(2);

        BLITZAR_CHECK(SeedCircularPair(particles, softening));

        blitzar_integration_comoving::AdvanceState<decltype(force_provider)> state{particles,
            accelerations, checkpoint, force_provider, start_time, timestep, background, settings,
            particles.State()};

        for (std::size_t step = 0; step < steps; ++step) {
            BLITZAR_CHECK(integrator.Advance(state) == BLITZAR_STATUS_OK);
        }

        const blitzar_core::ParticleStateView final = particles.State();

        out[0] = final.x[0];
        out[1] = final.y[0];
        out[2] = final.z[0];
        out[3] = final.x[1];
        out[4] = final.y[1];
        out[5] = final.z[1];

        return 0;
    };

    std::array<double, 6> coarse{};
    std::array<double, 6> medium{};
    std::array<double, 6> fine{};

    BLITZAR_CHECK(run_fixed(coarse_step, coarse_count, coarse) == 0);
    BLITZAR_CHECK(run_fixed(coarse_step * 0.5, coarse_count * 2, medium) == 0);
    BLITZAR_CHECK(run_fixed(coarse_step * 0.25, coarse_count * 4, fine) == 0);

    double coarse_error = 0.0;
    double medium_error = 0.0;

    for (std::size_t index = 0; index < coarse.size(); ++index) {
        const double coarse_diff = coarse[index] - medium[index];
        const double medium_diff = medium[index] - fine[index];

        coarse_error += coarse_diff * coarse_diff;
        medium_error += medium_diff * medium_diff;
    }

    const double order =
        std::log2(std::max(coarse_error, 1.0e-30) / std::max(medium_error, 1.0e-30)) * 0.5;

    BLITZAR_CHECK(order > 1.5);
    BLITZAR_CHECK(order < 2.5);

    return 0;
}

int CheckCanonicalMomentumConservation() noexcept
{
    constexpr double timestep = 0.002;
    constexpr double softening = 0.05;
    constexpr std::size_t steps = 512;
    constexpr double start_time = 1.0;

    const blitzar_integration::ComovingBackground background{
        blitzar_integration::ExpansionKind::EinsteinDeSitter, 1.0};

    const blitzar_core::ExecutionSettings settings{};
    const blitzar_physics::GravityParameters gravity{1.0, softening};
    blitzar_direct::DirectSolver solver(gravity);

    BLITZAR_CHECK(solver.Prepare(2) == BLITZAR_STATUS_OK);

    blitzar_solvers::SolverCpuForceProvider<blitzar_direct::DirectSolver> force_provider(solver);
    blitzar_particles::ParticleBuffer particles(2);
    blitzar_particles::ParticleAccelerationBuffer accelerations(2);
    blitzar_integration::KdkCheckpoint checkpoint(2);
    blitzar_integration::ComovingLeapfrog integrator{};

    BLITZAR_CHECK(SeedCircularPair(particles, softening));

    const blitzar_core::ParticleStateView initial = particles.State();
    const double initial_sum_x = initial.velocity_x[0] + initial.velocity_x[1];
    const double initial_sum_y = initial.velocity_y[0] + initial.velocity_y[1];
    const double initial_sum_z = initial.velocity_z[0] + initial.velocity_z[1];

    blitzar_integration_comoving::AdvanceState<decltype(force_provider)> state{particles,
        accelerations, checkpoint, force_provider, start_time, timestep, background, settings,
        particles.State()};

    for (std::size_t step = 0; step < steps; ++step) {
        BLITZAR_CHECK(integrator.Advance(state) == BLITZAR_STATUS_OK);
    }

    const blitzar_core::ParticleStateView final = particles.State();
    const double final_sum_x = final.velocity_x[0] + final.velocity_x[1];
    const double final_sum_y = final.velocity_y[0] + final.velocity_y[1];
    const double final_sum_z = final.velocity_z[0] + final.velocity_z[1];

    BLITZAR_CHECK(std::abs(final_sum_x - initial_sum_x) < 1.0e-12);
    BLITZAR_CHECK(std::abs(final_sum_y - initial_sum_y) < 1.0e-12);
    BLITZAR_CHECK(std::abs(final_sum_z - initial_sum_z) < 1.0e-12);

    return 0;
}

int CheckValidationRejects() noexcept
{
    const blitzar_integration::ComovingBackground static_background{};
    const blitzar_integration::ComovingBackground eds_background{
        blitzar_integration::ExpansionKind::EinsteinDeSitter, 1.0};

    const blitzar_core::ExecutionSettings settings{};
    const blitzar_physics::GravityParameters gravity{1.0, 0.05};
    blitzar_direct::DirectSolver solver(gravity);

    BLITZAR_CHECK(solver.Prepare(1) == BLITZAR_STATUS_OK);

    blitzar_solvers::SolverCpuForceProvider<blitzar_direct::DirectSolver> force_provider(solver);
    blitzar_integration::ComovingLeapfrog integrator{};

    {
        blitzar_particles::ParticleBuffer particles(1);
        blitzar_particles::ParticleAccelerationBuffer accelerations(1);
        blitzar_integration::KdkCheckpoint checkpoint(1);

        BLITZAR_CHECK(particles.SetVelocity(0, {1.0, 0.0, 0.0}) == BLITZAR_STATUS_OK);

        blitzar_integration_comoving::AdvanceState<decltype(force_provider)> state{particles,
            accelerations, checkpoint, force_provider, 1.0, 0.0, eds_background, settings,
            particles.State()};

        BLITZAR_CHECK(integrator.Advance(state) == BLITZAR_STATUS_INVALID_ARGUMENT);

        state.timestep = -0.5;

        BLITZAR_CHECK(integrator.Advance(state) == BLITZAR_STATUS_INVALID_ARGUMENT);

        state.timestep = std::numeric_limits<double>::infinity();

        BLITZAR_CHECK(integrator.Advance(state) == BLITZAR_STATUS_INVALID_ARGUMENT);
    }

    {
        blitzar_particles::ParticleBuffer particles(1);
        blitzar_particles::ParticleAccelerationBuffer accelerations(1);
        blitzar_integration::KdkCheckpoint checkpoint(1);

        blitzar_integration_comoving::AdvanceState<decltype(force_provider)> state{particles,
            accelerations, checkpoint, force_provider, 0.0, 0.5, eds_background, settings,
            particles.State()};

        BLITZAR_CHECK(integrator.Advance(state) == BLITZAR_STATUS_INVALID_ARGUMENT);
    }

    {
        blitzar_particles::ParticleBuffer particles(1);
        blitzar_particles::ParticleAccelerationBuffer accelerations(1);
        blitzar_integration::KdkCheckpoint checkpoint(1);

        const blitzar_integration::ComovingBackground invalid_background{
            blitzar_integration::ExpansionKind::EinsteinDeSitter, 0.0};

        blitzar_integration_comoving::AdvanceState<decltype(force_provider)> state{particles,
            accelerations, checkpoint, force_provider, 1.0, 0.5, invalid_background, settings,
            particles.State()};

        BLITZAR_CHECK(integrator.Advance(state) == BLITZAR_STATUS_INVALID_ARGUMENT);
    }

    {
        blitzar_particles::ParticleBuffer particles(1);
        blitzar_particles::ParticleAccelerationBuffer accelerations(1);
        blitzar_integration::KdkCheckpoint checkpoint(1);

        BLITZAR_CHECK(particles.SetVelocity(0, {1.0, 0.0, 0.0}) == BLITZAR_STATUS_OK);

        blitzar_integration_comoving::AdvanceState<decltype(force_provider)> state{particles,
            accelerations, checkpoint, force_provider, 5.0, 0.5, static_background, settings,
            particles.State()};

        BLITZAR_CHECK(integrator.Advance(state) == BLITZAR_STATUS_OK);
        BLITZAR_CHECK(state.cosmic_time == 5.5);
    }

    return 0;
}

int CheckRollbackOnForceFailure() noexcept
{
    const blitzar_integration::ComovingBackground static_background{};
    const blitzar_core::ExecutionSettings settings{};
    blitzar_particles::ParticleBuffer particles(1);
    blitzar_particles::ParticleAccelerationBuffer accelerations(1);
    blitzar_integration::KdkCheckpoint checkpoint(1);
    FailOnSecondSolver failing_solver{};
    blitzar_integration::ComovingLeapfrog integrator{};

    BLITZAR_CHECK(particles.SetPosition(0, {1.0, 2.0, 3.0}) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(particles.SetVelocity(0, {0.5, -1.0, 2.0}) == BLITZAR_STATUS_OK);

    blitzar_integration_comoving::AdvanceState<FailOnSecondSolver> state{particles, accelerations,
        checkpoint, failing_solver, 1.0, 0.5, static_background, settings, particles.State()};

    BLITZAR_CHECK(integrator.Advance(state) == BLITZAR_STATUS_INTERNAL_ERROR);

    const blitzar_core::ParticleStateView restored = particles.State();

    BLITZAR_CHECK(restored.x[0] == 1.0);
    BLITZAR_CHECK(restored.y[0] == 2.0);
    BLITZAR_CHECK(restored.z[0] == 3.0);
    BLITZAR_CHECK(restored.velocity_x[0] == 0.5);
    BLITZAR_CHECK(restored.velocity_y[0] == -1.0);
    BLITZAR_CHECK(restored.velocity_z[0] == 2.0);
    BLITZAR_CHECK(state.cosmic_time == 1.0);

    return 0;
}

} // namespace

int main()
{
    BLITZAR_CHECK(CheckStaticParityWithKdk() == 0);
    BLITZAR_CHECK(CheckEmptyUniverseExpandingOracle() == 0);
    BLITZAR_CHECK(CheckExpandingTwoBodyConvergence() == 0);
    BLITZAR_CHECK(CheckCanonicalMomentumConservation() == 0);
    BLITZAR_CHECK(CheckValidationRejects() == 0);
    BLITZAR_CHECK(CheckRollbackOnForceFailure() == 0);

    return 0;
}
