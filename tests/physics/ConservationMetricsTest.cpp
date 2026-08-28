#include "physics/conservation/ConservationMetrics.hpp"

#include "core/CoreExecution.hpp"
#include "fixtures/FixtureCheck.hpp"
#include "fixtures/FixtureForce.hpp"
#include "particles/buffer/ParticleAccelerationBuffer.hpp"
#include "particles/buffer/ParticleBuffer.hpp"
#include "solvers/barnes_hut/BhSolver.hpp"
#include "solvers/direct/DirectSolver.hpp"
#include "solvers/fmm/FmmSolver.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <span>

namespace {

bool InitializeParticles(blitzar_particles::ParticleBuffer& particles) noexcept
{
    const std::array<blitzar_core::Vector3, 3> positions{blitzar_core::Vector3{-1.0, 0.0, 0.0},
        blitzar_core::Vector3{1.0, 0.0, 0.0}, blitzar_core::Vector3{0.0, 1.0, 0.0}};

    for (std::size_t index = 0; index < positions.size(); ++index) {
        if (particles.SetPosition(index, positions[index]) != BLITZAR_STATUS_OK ||
            particles.SetVelocity(index, {}) != BLITZAR_STATUS_OK ||

            particles.SetMass(index, 1.0 + static_cast<double>(index)) != BLITZAR_STATUS_OK) {
            return false;
        }
    }

    return true;
}

bool Near(double actual, double expected, double tolerance) noexcept
{
    return std::abs(actual - expected) <= tolerance;
}

bool RunValueCase() noexcept
{
    blitzar_particles::ParticleBuffer particles(2);

    if (particles.SetPosition(0, {0.0, 0.0, 0.0}) != BLITZAR_STATUS_OK) {
        return false;
    }
    if (particles.SetPosition(1, {1.0, 0.0, 0.0}) != BLITZAR_STATUS_OK) {
        return false;
    }
    if (particles.SetVelocity(0, {2.0, 0.0, 0.0}) != BLITZAR_STATUS_OK) {
        return false;
    }
    if (particles.SetVelocity(1, {-1.0, 0.0, 0.0}) != BLITZAR_STATUS_OK) {
        return false;
    }
    if (particles.SetMass(0, 2.0) != BLITZAR_STATUS_OK) {
        return false;
    }
    if (particles.SetMass(1, 3.0) != BLITZAR_STATUS_OK) {
        return false;
    }

    const blitzar_physics::GravityParameters gravity{2.0, 0.5, {2.0, 3.0, 4.0}};

    blitzar_physics::ConservationMetrics metrics{};

    if (blitzar_physics::ComputeConservationMetrics(particles.State(), gravity, metrics) !=
        BLITZAR_STATUS_OK) {
        return false;
    }

    const double softened_distance = std::sqrt(1.0 + 0.25 * 0.25);
    const double expected_potential = -72.0 / softened_distance;

    const std::array<bool, 7> checks{Near(metrics.kinetic_energy, 5.5, 1.0e-12),
        Near(metrics.potential_energy, expected_potential, 1.0e-12),
        Near(metrics.total_energy, 5.5 + expected_potential, 1.0e-12),
        Near(metrics.momentum.x, 1.0, 1.0e-12), Near(metrics.momentum.y, 0.0, 1.0e-12),
        Near(metrics.momentum.z, 0.0, 1.0e-12), metrics.IsFinite()};

    for (const bool check : checks) {
        if (!check) {
            return false;
        }
    }

    return true;
}

bool RunEmptyCase() noexcept
{
    blitzar_particles::ParticleBuffer particles(0);
    blitzar_physics::ConservationMetrics metrics{};

    if (blitzar_physics::ComputeConservationMetrics(particles.State(), {1.0, 0.1}, metrics) !=

        BLITZAR_STATUS_OK) {
        return false;
    }

    return metrics.kinetic_energy == 0.0 && metrics.potential_energy == 0.0 &&
           metrics.total_energy == 0.0 && metrics.momentum.x == 0.0 && metrics.momentum.y == 0.0 &&
           metrics.momentum.z == 0.0;
}

bool RunSingularityCase() noexcept
{
    blitzar_particles::ParticleBuffer particles(2);
    const blitzar_physics::ConservationMetrics before{3.0, 4.0, 7.0, {8.0, 9.0, 10.0}};
    blitzar_physics::ConservationMetrics metrics = before;

    if (particles.SetPosition(0, {0.0, 0.0, 0.0}) != BLITZAR_STATUS_OK ||

        particles.SetPosition(1, {0.0, 0.0, 0.0}) != BLITZAR_STATUS_OK ||
        blitzar_physics::ComputeConservationMetrics(particles.State(), {1.0, 0.0}, metrics) !=
            BLITZAR_STATUS_SINGULARITY) {
        return false;
    }

    return metrics.kinetic_energy == before.kinetic_energy &&
           metrics.potential_energy == before.potential_energy &&
           metrics.total_energy == before.total_energy && metrics.momentum.x == before.momentum.x &&
           metrics.momentum.y == before.momentum.y && metrics.momentum.z == before.momentum.z;
}

bool RunZeroMassCase() noexcept
{
    blitzar_particles::ParticleBuffer particles(2);
    blitzar_physics::ConservationMetrics metrics{};

    if (particles.SetPosition(0, {0.0, 0.0, 0.0}) != BLITZAR_STATUS_OK ||

        particles.SetPosition(1, {0.0, 0.0, 0.0}) != BLITZAR_STATUS_OK ||
        particles.SetMass(0, 1.0) != BLITZAR_STATUS_OK ||
        particles.SetMass(1, 0.0) != BLITZAR_STATUS_OK ||
        blitzar_physics::ComputeConservationMetrics(particles.State(), {1.0, 0.0}, metrics) !=
            BLITZAR_STATUS_OK) {
        return false;
    }

    return metrics.kinetic_energy == 0.0 && metrics.potential_energy == 0.0 &&
           metrics.total_energy == 0.0 && metrics.momentum.x == 0.0 && metrics.momentum.y == 0.0 &&
           metrics.momentum.z == 0.0;
}

bool RunExtendedSourceCase() noexcept
{
    const std::array<double, 2> values{1.0, 2.0};
    const std::span<const double> spans(values);
    const blitzar_core::ParticleStateView state{
        1, spans, spans, spans, spans, spans, spans, spans, 2};

    const blitzar_physics::ConservationMetrics before{1.0, 2.0, 3.0, {4.0, 5.0, 6.0}};
    blitzar_physics::ConservationMetrics metrics = before;

    if (blitzar_physics::ComputeConservationMetrics(state, {1.0, 0.1}, metrics) !=

        BLITZAR_STATUS_INVALID_ARGUMENT) {
        return false;
    }

    return metrics.kinetic_energy == before.kinetic_energy &&
           metrics.potential_energy == before.potential_energy &&
           metrics.total_energy == before.total_energy && metrics.momentum.x == before.momentum.x &&
           metrics.momentum.y == before.momentum.y && metrics.momentum.z == before.momentum.z;
}

bool RunInvalidStateCase() noexcept
{
    const std::array<double, 1> invalid{std::numeric_limits<double>::quiet_NaN()};
    const std::span<const double> values(invalid);
    const blitzar_core::ParticleStateView state{
        1, values, values, values, values, values, values, values};

    const blitzar_physics::ConservationMetrics before{1.0, 2.0, 3.0, {4.0, 5.0, 6.0}};
    blitzar_physics::ConservationMetrics metrics = before;

    if (blitzar_physics::ComputeConservationMetrics(state, {1.0, 0.1}, metrics) !=

        BLITZAR_STATUS_INVALID_ARGUMENT) {
        return false;
    }

    return metrics.kinetic_energy == before.kinetic_energy &&
           metrics.potential_energy == before.potential_energy &&
           metrics.total_energy == before.total_energy && metrics.momentum.x == before.momentum.x &&
           metrics.momentum.y == before.momentum.y && metrics.momentum.z == before.momentum.z;
}

bool RunInvalidParametersCase() noexcept
{
    blitzar_particles::ParticleBuffer particles(1);
    const blitzar_physics::ConservationMetrics before{1.0, 2.0, 3.0, {4.0, 5.0, 6.0}};
    blitzar_physics::ConservationMetrics metrics = before;

    if (blitzar_physics::ComputeConservationMetrics(particles.State(), {0.0, 0.1}, metrics) !=

        BLITZAR_STATUS_INVALID_ARGUMENT) {
        return false;
    }

    return metrics.kinetic_energy == before.kinetic_energy &&
           metrics.potential_energy == before.potential_energy &&
           metrics.total_energy == before.total_energy && metrics.momentum.x == before.momentum.x &&
           metrics.momentum.y == before.momentum.y && metrics.momentum.z == before.momentum.z;
}

template <typename Solver>
bool RunSolverCase(Solver& solver, blitzar_particles::ParticleBuffer& particles,
    blitzar_particles::ParticleAccelerationBuffer& accelerations,
    const blitzar_physics::GravityParameters& gravity) noexcept
{
    const blitzar_core::ExecutionSettings settings{};
    blitzar_physics::ConservationMetrics metrics{};

    return solver.Prepare(particles.Count()) == BLITZAR_STATUS_OK &&
           blitzar_tests::EvaluateLocal(
               solver, particles.State(), accelerations.View(), settings) == BLITZAR_STATUS_OK &&
           blitzar_physics::ComputeConservationMetrics(particles.State(), gravity, metrics) ==
               BLITZAR_STATUS_OK &&
           metrics.IsFinite();
}

bool RunSolverCases() noexcept
{
    const blitzar_physics::GravityParameters gravity{1.0, 0.1};
    const blitzar_barnes_hut::BarnesHutSettings settings{0.0, 3, 64, 2, 8};
    blitzar_particles::ParticleBuffer particles(3);
    blitzar_particles::ParticleAccelerationBuffer direct_accelerations(3);
    blitzar_particles::ParticleAccelerationBuffer bh_accelerations(3);
    blitzar_particles::ParticleAccelerationBuffer fmm_accelerations(3);
    blitzar_direct::DirectSolver direct(gravity, 3);
    blitzar_solvers::SolverTreeResources bh_resources(
        {3, settings.max_cells, settings.leaf_capacity, settings.max_depth},
        {settings.max_particles, settings.max_cells, settings.leaf_capacity, settings.max_depth});

    blitzar_solvers::SolverTreeResources fmm_resources(
        {3, settings.max_cells, settings.leaf_capacity, settings.max_depth},
        {settings.max_particles, settings.max_cells, settings.leaf_capacity, settings.max_depth});

    blitzar_barnes_hut::BhSolver barnes_hut(gravity, settings, 3, bh_resources);
    blitzar_fmm::FmmSolver fmm(gravity, settings, 3, fmm_resources);

    return InitializeParticles(particles) &&
           RunSolverCase(direct, particles, direct_accelerations, gravity) &&
           RunSolverCase(barnes_hut, particles, bh_accelerations, gravity) &&
           RunSolverCase(fmm, particles, fmm_accelerations, gravity);
}

} // namespace

int main()
{
    BLITZAR_CHECK(RunValueCase());
    BLITZAR_CHECK(RunEmptyCase());
    BLITZAR_CHECK(RunSingularityCase());
    BLITZAR_CHECK(RunZeroMassCase());
    BLITZAR_CHECK(RunExtendedSourceCase());
    BLITZAR_CHECK(RunInvalidStateCase());
    BLITZAR_CHECK(RunInvalidParametersCase());
    BLITZAR_CHECK(RunSolverCases());

    return 0;
}
