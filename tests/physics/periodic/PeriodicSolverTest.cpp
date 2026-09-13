#include "core/CoreExecution.hpp"
#include "core/CoreTypes.hpp"
#include "fixtures/FixtureCheck.hpp"
#include "particles/buffer/ParticleAccelerationBuffer.hpp"
#include "particles/buffer/ParticleBuffer.hpp"
#include "physics/gravity/GravityLaw.hpp"
#include "physics/periodic/PeriodicDomain.hpp"
#include "solvers/SolverCpuForceProvider.hpp"
#include "solvers/SolverForceEvaluation.hpp"
#include "solvers/SolverTreeResources.hpp"
#include "solvers/bh/BhSolver.hpp"
#include "solvers/direct/DirectSolver.hpp"
#include "solvers/fmm/FmmSolver.hpp"
#include "solvers/fmm/kifmm/KifmmSolver.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace {

template <typename Settings>
[[nodiscard]] Settings MakeSettings(
    std::size_t particle_count, double opening_angle, std::size_t leaf_capacity) noexcept
{
    return {opening_angle, particle_count, particle_count * 8 + 1, leaf_capacity, 16};
}

template <typename Settings>
[[nodiscard]] blitzar_solvers::SolverTreeResources MakeResources(
    Settings settings, std::size_t local_capacity) noexcept
{
    return {{local_capacity, settings.max_cells, settings.leaf_capacity, settings.max_depth},
        {settings.max_particles, settings.max_cells, settings.leaf_capacity, settings.max_depth}};
}

template <typename Solver>
[[nodiscard]] blitzar_status EvaluatePeriodic(Solver& solver,
    blitzar_core::ParticleStateView particles, blitzar_core::ForceView forces,
    const blitzar_physics::PeriodicDomain* periodic) noexcept
{
    blitzar_solvers::SolverCpuForceProvider<Solver> provider(solver);
    const blitzar_solvers::SolverForceEvaluation request{particles, particles, forces,
        blitzar_core::ExecutionSettings{}, blitzar_solvers::SolverForceSourceKind::Local, periodic};

    return provider.Evaluate(request);
}

void FillParticles(blitzar_particles::ParticleBuffer& particles, bool tight) noexcept
{
    for (std::size_t index = 0; index < particles.Count(); ++index) {
        blitzar_core::Vector3 position{};

        if (tight) {
            position = {-0.2 + 0.4 * static_cast<double>(index % 2),
                -0.2 + 0.4 * static_cast<double>((index / 2) % 2),
                -0.2 + 0.4 * static_cast<double>(index / 4)};
        }
        else {
            position = {-0.75 + 0.5 * static_cast<double>(index % 4),
                -0.75 + 0.5 * static_cast<double>((index / 4) % 4),
                -0.75 + 0.5 * static_cast<double>(index / 16)};
        }

        (void)particles.SetPosition(index, position);
        (void)particles.SetVelocity(index, {});
        (void)particles.SetMass(index, 1.0 + 0.25 * static_cast<double>(index % 3));
    }
}

[[nodiscard]] double ForceRelativeError(
    blitzar_core::ForceView expected, blitzar_core::ForceView actual) noexcept
{
    double maximum = 0.0;

    for (std::size_t index = 0; index < expected.count; ++index) {
        const double dx = expected.x[index] - actual.x[index];
        const double dy = expected.y[index] - actual.y[index];
        const double dz = expected.z[index] - actual.z[index];
        const double expected_norm = std::sqrt(expected.x[index] * expected.x[index] +
                                               expected.y[index] * expected.y[index] +
                                               expected.z[index] * expected.z[index]);

        const double error_norm = std::sqrt(dx * dx + dy * dy + dz * dz);

        maximum = std::max(maximum, error_norm / std::max(1.0, expected_norm));
    }

    return maximum;
}

[[nodiscard]] bool RunWindowEquivalentCase() noexcept
{
    constexpr std::size_t ParticleCount = 8;
    const blitzar_physics::PeriodicDomain periodic{
        true, blitzar_core::Vector3{-1.0, -1.0, -1.0}, blitzar_core::Vector3{2.0, 2.0, 2.0}};

    const blitzar_physics::GravityParameters gravity{1.0, 0.05};
    blitzar_particles::ParticleBuffer particles(ParticleCount);
    blitzar_particles::ParticleAccelerationBuffer direct_force(ParticleCount);
    blitzar_particles::ParticleAccelerationBuffer windowed_force(ParticleCount);

    FillParticles(particles, true);

    blitzar_direct::DirectSolver direct(gravity, ParticleCount);

    if (EvaluatePeriodic(direct, particles.State(), direct_force.View(), &periodic) !=
        BLITZAR_STATUS_OK) {
        return false;
    }

    const blitzar_core::ParticleStateView state = particles.State();
    blitzar_core::ForceView windowed = windowed_force.View();

    for (std::size_t target = 0; target < state.count; ++target) {
        double ax = 0.0;
        double ay = 0.0;
        double az = 0.0;

        for (std::size_t source = 0; source < state.count; ++source) {
            if (source == target) {
                continue;
            }

            for (std::size_t region = 0; region < periodic.ImageCount(); ++region) {
                const blitzar_core::Vector3 shift = periodic.ImageShift(region);
                const double dx = state.x[source] - state.x[target] - shift.x;
                const double dy = state.y[source] - state.y[target] - shift.y;
                const double dz = state.z[source] - state.z[target] - shift.z;
                const double distance_squared = dx * dx + dy * dy + dz * dz;

                if (distance_squared > 1.0) {
                    continue;
                }

                const double softened = distance_squared + 0.05 * 0.05;
                const double factor = state.mass[source] / (softened * std::sqrt(softened));

                ax += factor * dx;
                ay += factor * dy;
                az += factor * dz;
            }
        }

        windowed.x[target] = ax;
        windowed.y[target] = ay;
        windowed.z[target] = az;
    }

    return ForceRelativeError(direct_force.View(), windowed_force.View()) < 1.0e-9;
}

[[nodiscard]] bool RunExactParityCase() noexcept
{
    constexpr std::size_t ParticleCount = 8;
    const blitzar_physics::PeriodicDomain periodic{
        true, blitzar_core::Vector3{-1.0, -1.0, -1.0}, blitzar_core::Vector3{2.0, 2.0, 2.0}};

    const blitzar_physics::GravityParameters gravity{1.0, 0.05};
    blitzar_particles::ParticleBuffer particles(ParticleCount);
    blitzar_particles::ParticleAccelerationBuffer direct_force(ParticleCount);
    blitzar_particles::ParticleAccelerationBuffer bh_force(ParticleCount);
    blitzar_particles::ParticleAccelerationBuffer fmm_force(ParticleCount);
    blitzar_particles::ParticleAccelerationBuffer kifmm_force(ParticleCount);

    FillParticles(particles, true);

    blitzar_direct::DirectSolver direct(gravity, ParticleCount);

    if (EvaluatePeriodic(direct, particles.State(), direct_force.View(), &periodic) !=
        BLITZAR_STATUS_OK) {
        return false;
    }

    const auto bh_settings =
        MakeSettings<blitzar_barnes_hut::BarnesHutSettings>(ParticleCount, 0.0, ParticleCount);

    auto bh_resources = MakeResources(bh_settings, ParticleCount);
    blitzar_barnes_hut::BhSolver barnes_hut(gravity, bh_settings, ParticleCount, bh_resources);

    if (EvaluatePeriodic(barnes_hut, particles.State(), bh_force.View(), &periodic) !=
            BLITZAR_STATUS_OK ||
        ForceRelativeError(direct_force.View(), bh_force.View()) != 0.0) {
        return false;
    }

    const auto fmm_settings =
        MakeSettings<blitzar_fmm::FmmSettings>(ParticleCount, 0.0, ParticleCount);

    auto fmm_resources = MakeResources(fmm_settings, ParticleCount);
    blitzar_fmm::FmmSolver fmm(gravity, fmm_settings, ParticleCount, fmm_resources);

    if (EvaluatePeriodic(fmm, particles.State(), fmm_force.View(), &periodic) !=
            BLITZAR_STATUS_OK ||
        ForceRelativeError(direct_force.View(), fmm_force.View()) != 0.0) {
        return false;
    }

    const auto kifmm_settings =
        MakeSettings<blitzar_kifmm::KifmmSettings>(ParticleCount, 0.0, ParticleCount);

    auto kifmm_resources = MakeResources(kifmm_settings, ParticleCount);
    blitzar_kifmm::KifmmSolver kifmm(gravity, kifmm_settings, ParticleCount, kifmm_resources);

    if (EvaluatePeriodic(kifmm, particles.State(), kifmm_force.View(), &periodic) !=
            BLITZAR_STATUS_OK ||
        ForceRelativeError(direct_force.View(), kifmm_force.View()) > 1.0e-12) {
        return false;
    }

    return true;
}

[[nodiscard]] bool RunApproximateParityCase() noexcept
{
    constexpr std::size_t ParticleCount = 64;
    const blitzar_physics::PeriodicDomain periodic{
        true, blitzar_core::Vector3{-1.0, -1.0, -1.0}, blitzar_core::Vector3{2.0, 2.0, 2.0}};

    const blitzar_physics::GravityParameters gravity{1.0, 0.1};
    blitzar_particles::ParticleBuffer particles(ParticleCount);
    blitzar_particles::ParticleAccelerationBuffer direct_force(ParticleCount);
    blitzar_particles::ParticleAccelerationBuffer bh_force(ParticleCount);
    blitzar_particles::ParticleAccelerationBuffer fmm_force(ParticleCount);
    blitzar_particles::ParticleAccelerationBuffer kifmm_force(ParticleCount);

    FillParticles(particles, false);

    blitzar_direct::DirectSolver direct(gravity, ParticleCount);

    if (EvaluatePeriodic(direct, particles.State(), direct_force.View(), &periodic) !=
        BLITZAR_STATUS_OK) {
        return false;
    }

    const auto bh_settings =
        MakeSettings<blitzar_barnes_hut::BarnesHutSettings>(ParticleCount, 0.35, 16);

    auto bh_resources = MakeResources(bh_settings, ParticleCount);
    blitzar_barnes_hut::BhSolver barnes_hut(gravity, bh_settings, ParticleCount, bh_resources);

    if (EvaluatePeriodic(barnes_hut, particles.State(), bh_force.View(), &periodic) !=
            BLITZAR_STATUS_OK ||
        ForceRelativeError(direct_force.View(), bh_force.View()) > 0.05) {
        return false;
    }

    const auto fmm_settings = MakeSettings<blitzar_fmm::FmmSettings>(ParticleCount, 0.35, 16);

    auto fmm_resources = MakeResources(fmm_settings, ParticleCount);
    blitzar_fmm::FmmSolver fmm(gravity, fmm_settings, ParticleCount, fmm_resources);

    if (EvaluatePeriodic(fmm, particles.State(), fmm_force.View(), &periodic) !=
            BLITZAR_STATUS_OK ||
        ForceRelativeError(direct_force.View(), fmm_force.View()) > 0.05) {
        return false;
    }

    const auto kifmm_settings = MakeSettings<blitzar_kifmm::KifmmSettings>(ParticleCount, 0.35, 16);

    auto kifmm_resources = MakeResources(kifmm_settings, ParticleCount);
    blitzar_kifmm::KifmmSolver kifmm(gravity, kifmm_settings, ParticleCount, kifmm_resources);

    if (EvaluatePeriodic(kifmm, particles.State(), kifmm_force.View(), &periodic) !=
            BLITZAR_STATUS_OK ||
        ForceRelativeError(direct_force.View(), kifmm_force.View()) > 0.05) {
        return false;
    }

    return true;
}

[[nodiscard]] bool RunSeamNoDoubleCountCase() noexcept
{
    constexpr std::size_t ParticleCount = 2;
    const blitzar_physics::PeriodicDomain periodic{
        true, blitzar_core::Vector3{-1.0, -1.0, -1.0}, blitzar_core::Vector3{2.0, 2.0, 2.0}};

    const blitzar_physics::GravityParameters gravity{1.0, 0.05};
    blitzar_particles::ParticleBuffer particles(ParticleCount);
    blitzar_particles::ParticleAccelerationBuffer periodic_force(ParticleCount);
    blitzar_particles::ParticleAccelerationBuffer expected_force(ParticleCount);
    blitzar_particles::ParticleAccelerationBuffer open_force(ParticleCount);
    blitzar_particles::ParticleAccelerationBuffer bh_force(ParticleCount);

    (void)particles.SetPosition(0, {-0.99, 0.0, 0.0});
    (void)particles.SetPosition(1, {0.99, 0.0, 0.0});
    (void)particles.SetVelocity(0, {});
    (void)particles.SetVelocity(1, {});
    (void)particles.SetMass(0, 1.0);
    (void)particles.SetMass(1, 1.0);

    blitzar_direct::DirectSolver direct(gravity, ParticleCount);

    if (EvaluatePeriodic(direct, particles.State(), periodic_force.View(), &periodic) !=
            BLITZAR_STATUS_OK ||
        EvaluatePeriodic(direct, particles.State(), open_force.View(), nullptr) !=
            BLITZAR_STATUS_OK) {
        return false;
    }

    const double softened = 0.02 * 0.02 + 0.05 * 0.05;
    const double factor = 1.0 / (softened * std::sqrt(softened));
    const blitzar_core::ForceView expected = expected_force.View();

    expected.x[0] = -factor * 0.02;
    expected.y[0] = 0.0;
    expected.z[0] = 0.0;
    expected.x[1] = factor * 0.02;
    expected.y[1] = 0.0;
    expected.z[1] = 0.0;

    const auto bh_settings =
        MakeSettings<blitzar_barnes_hut::BarnesHutSettings>(ParticleCount, 0.0, ParticleCount);

    auto bh_resources = MakeResources(bh_settings, ParticleCount);
    blitzar_barnes_hut::BhSolver barnes_hut(gravity, bh_settings, ParticleCount, bh_resources);

    if (ForceRelativeError(periodic_force.View(), expected_force.View()) > 1.0e-9 ||
        ForceRelativeError(open_force.View(), expected_force.View()) < 0.5 ||
        EvaluatePeriodic(barnes_hut, particles.State(), bh_force.View(), &periodic) !=
            BLITZAR_STATUS_OK ||
        ForceRelativeError(periodic_force.View(), bh_force.View()) != 0.0) {
        return false;
    }

    return true;
}

[[nodiscard]] bool RunOpenBoundaryUnchangedCase() noexcept
{
    constexpr std::size_t ParticleCount = 64;
    const blitzar_physics::PeriodicDomain disabled{};
    const blitzar_physics::GravityParameters gravity{1.0, 0.1};
    blitzar_particles::ParticleBuffer particles(ParticleCount);
    blitzar_particles::ParticleAccelerationBuffer untouched(ParticleCount);
    blitzar_particles::ParticleAccelerationBuffer opened(ParticleCount);

    FillParticles(particles, false);

    blitzar_direct::DirectSolver direct(gravity, ParticleCount);

    if (EvaluatePeriodic(direct, particles.State(), untouched.View(), &disabled) !=
            BLITZAR_STATUS_OK ||
        EvaluatePeriodic(direct, particles.State(), opened.View(), nullptr) != BLITZAR_STATUS_OK ||
        ForceRelativeError(untouched.View(), opened.View()) != 0.0) {
        return false;
    }

    const auto bh_settings =
        MakeSettings<blitzar_barnes_hut::BarnesHutSettings>(ParticleCount, 0.35, 16);

    auto bh_resources = MakeResources(bh_settings, ParticleCount);
    blitzar_barnes_hut::BhSolver barnes_hut(gravity, bh_settings, ParticleCount, bh_resources);

    if (EvaluatePeriodic(barnes_hut, particles.State(), untouched.View(), &disabled) !=
            BLITZAR_STATUS_OK ||
        EvaluatePeriodic(barnes_hut, particles.State(), opened.View(), nullptr) !=
            BLITZAR_STATUS_OK ||
        ForceRelativeError(untouched.View(), opened.View()) != 0.0) {
        return false;
    }

    const auto fmm_settings = MakeSettings<blitzar_fmm::FmmSettings>(ParticleCount, 0.35, 16);

    auto fmm_resources = MakeResources(fmm_settings, ParticleCount);
    blitzar_fmm::FmmSolver fmm(gravity, fmm_settings, ParticleCount, fmm_resources);

    if (EvaluatePeriodic(fmm, particles.State(), untouched.View(), &disabled) !=
            BLITZAR_STATUS_OK ||
        EvaluatePeriodic(fmm, particles.State(), opened.View(), nullptr) != BLITZAR_STATUS_OK ||
        ForceRelativeError(untouched.View(), opened.View()) != 0.0) {
        return false;
    }

    const auto kifmm_settings = MakeSettings<blitzar_kifmm::KifmmSettings>(ParticleCount, 0.35, 16);

    auto kifmm_resources = MakeResources(kifmm_settings, ParticleCount);
    blitzar_kifmm::KifmmSolver kifmm(gravity, kifmm_settings, ParticleCount, kifmm_resources);

    if (EvaluatePeriodic(kifmm, particles.State(), untouched.View(), &disabled) !=
            BLITZAR_STATUS_OK ||
        EvaluatePeriodic(kifmm, particles.State(), opened.View(), nullptr) != BLITZAR_STATUS_OK ||
        ForceRelativeError(untouched.View(), opened.View()) != 0.0) {
        return false;
    }

    return true;
}

[[nodiscard]] bool RunRefitParityCase() noexcept
{
    constexpr std::size_t ParticleCount = 64;
    const blitzar_physics::PeriodicDomain periodic{
        true, blitzar_core::Vector3{-1.0, -1.0, -1.0}, blitzar_core::Vector3{2.0, 2.0, 2.0}};

    const blitzar_physics::GravityParameters gravity{1.0, 0.1};
    blitzar_particles::ParticleBuffer particles(ParticleCount);
    blitzar_particles::ParticleAccelerationBuffer first_force(ParticleCount);
    blitzar_particles::ParticleAccelerationBuffer second_force(ParticleCount);

    FillParticles(particles, false);

    const auto bh_settings =
        MakeSettings<blitzar_barnes_hut::BarnesHutSettings>(ParticleCount, 0.35, 16);

    auto bh_resources = MakeResources(bh_settings, ParticleCount);
    blitzar_barnes_hut::BhSolver barnes_hut(gravity, bh_settings, ParticleCount, bh_resources);

    if (EvaluatePeriodic(barnes_hut, particles.State(), first_force.View(), &periodic) !=
            BLITZAR_STATUS_OK ||
        EvaluatePeriodic(barnes_hut, particles.State(), second_force.View(), &periodic) !=
            BLITZAR_STATUS_OK ||
        ForceRelativeError(first_force.View(), second_force.View()) != 0.0) {
        return false;
    }

    const auto fmm_settings = MakeSettings<blitzar_fmm::FmmSettings>(ParticleCount, 0.35, 16);

    auto fmm_resources = MakeResources(fmm_settings, ParticleCount);
    blitzar_fmm::FmmSolver fmm(gravity, fmm_settings, ParticleCount, fmm_resources);

    if (EvaluatePeriodic(fmm, particles.State(), first_force.View(), &periodic) !=
            BLITZAR_STATUS_OK ||
        EvaluatePeriodic(fmm, particles.State(), second_force.View(), &periodic) !=
            BLITZAR_STATUS_OK ||
        ForceRelativeError(first_force.View(), second_force.View()) != 0.0) {
        return false;
    }

    const auto kifmm_settings = MakeSettings<blitzar_kifmm::KifmmSettings>(ParticleCount, 0.35, 16);

    auto kifmm_resources = MakeResources(kifmm_settings, ParticleCount);
    blitzar_kifmm::KifmmSolver kifmm(gravity, kifmm_settings, ParticleCount, kifmm_resources);

    if (EvaluatePeriodic(kifmm, particles.State(), first_force.View(), &periodic) !=
            BLITZAR_STATUS_OK ||
        EvaluatePeriodic(kifmm, particles.State(), second_force.View(), &periodic) !=
            BLITZAR_STATUS_OK ||
        ForceRelativeError(first_force.View(), second_force.View()) != 0.0) {
        return false;
    }

    return true;
}

} // namespace

int main()
{
    BLITZAR_CHECK(RunWindowEquivalentCase());
    BLITZAR_CHECK(RunExactParityCase());
    BLITZAR_CHECK(RunApproximateParityCase());
    BLITZAR_CHECK(RunSeamNoDoubleCountCase());
    BLITZAR_CHECK(RunOpenBoundaryUnchangedCase());
    BLITZAR_CHECK(RunRefitParityCase());

    return 0;
}
