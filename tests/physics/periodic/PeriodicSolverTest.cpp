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

constexpr std::size_t ExactParticleCount = 8;
constexpr std::size_t LatticeParticleCount = 64;
constexpr std::size_t OpenLeafCapacity = 16;
constexpr double ExactOpeningAngle = 0.0;
constexpr double DefaultOpeningAngle = 0.35;
constexpr double ExactSoftening = 0.05;
constexpr double ApproximateSoftening = 0.1;
constexpr double ApproximateRelativeTolerance = 0.05;
constexpr double KifmmExactRelativeBound = 1.0e-12;
constexpr double WindowRadiusSquared = 1.0;

constexpr blitzar_core::Vector3 BoxOrigin{-1.0, -1.0, -1.0};
constexpr blitzar_core::Vector3 BoxSize{2.0, 2.0, 2.0};

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

struct SolverCase final {
    blitzar_core::ParticleStateView particles;
    const blitzar_physics::GravityParameters& gravity;
    std::size_t particle_count;
    double opening_angle;
    std::size_t leaf_capacity;
};

template <typename Settings, typename Solver>
[[nodiscard]] bool EvaluateSolverCase(const SolverCase& config, blitzar_core::ForceView forces,
    const blitzar_physics::PeriodicDomain* periodic) noexcept
{
    const auto settings =
        MakeSettings<Settings>(config.particle_count, config.opening_angle, config.leaf_capacity);

    auto resources = MakeResources(settings, config.particle_count);
    Solver solver(config.gravity, settings, config.particle_count, resources);

    return EvaluatePeriodic(solver, config.particles, forces, periodic) == BLITZAR_STATUS_OK;
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
    const blitzar_physics::PeriodicDomain periodic{true, BoxOrigin, BoxSize};
    const blitzar_physics::GravityParameters gravity{1.0, ExactSoftening};
    blitzar_particles::ParticleBuffer particles(ExactParticleCount);
    blitzar_particles::ParticleAccelerationBuffer direct_force(ExactParticleCount);
    blitzar_particles::ParticleAccelerationBuffer windowed_force(ExactParticleCount);

    FillParticles(particles, true);

    blitzar_direct::DirectSolver direct(gravity, ExactParticleCount);

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

                if (distance_squared > WindowRadiusSquared) {
                    continue;
                }

                const double softened = distance_squared + ExactSoftening * ExactSoftening;
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
    const blitzar_physics::PeriodicDomain periodic{true, BoxOrigin, BoxSize};
    const blitzar_physics::GravityParameters gravity{1.0, ExactSoftening};
    blitzar_particles::ParticleBuffer particles(ExactParticleCount);
    blitzar_particles::ParticleAccelerationBuffer direct_force(ExactParticleCount);
    blitzar_particles::ParticleAccelerationBuffer bh_force(ExactParticleCount);
    blitzar_particles::ParticleAccelerationBuffer fmm_force(ExactParticleCount);
    blitzar_particles::ParticleAccelerationBuffer kifmm_force(ExactParticleCount);

    FillParticles(particles, true);

    const SolverCase config{
        particles.State(), gravity, ExactParticleCount, ExactOpeningAngle, ExactParticleCount};

    blitzar_direct::DirectSolver direct(gravity, ExactParticleCount);

    if (EvaluatePeriodic(direct, particles.State(), direct_force.View(), &periodic) !=
        BLITZAR_STATUS_OK) {
        return false;
    }

    if (!EvaluateSolverCase<blitzar_barnes_hut::BarnesHutSettings, blitzar_barnes_hut::BhSolver>(
            config, bh_force.View(), &periodic) ||
        ForceRelativeError(direct_force.View(), bh_force.View()) != 0.0) {
        return false;
    }

    if (!EvaluateSolverCase<blitzar_fmm::FmmSettings, blitzar_fmm::FmmSolver>(
            config, fmm_force.View(), &periodic) ||
        ForceRelativeError(direct_force.View(), fmm_force.View()) != 0.0) {
        return false;
    }

    if (!EvaluateSolverCase<blitzar_kifmm::KifmmSettings, blitzar_kifmm::KifmmSolver>(
            config, kifmm_force.View(), &periodic) ||
        ForceRelativeError(direct_force.View(), kifmm_force.View()) > KifmmExactRelativeBound) {
        return false;
    }

    return true;
}

[[nodiscard]] bool RunApproximateParityCase() noexcept
{
    const blitzar_physics::PeriodicDomain periodic{true, BoxOrigin, BoxSize};
    const blitzar_physics::GravityParameters gravity{1.0, ApproximateSoftening};
    blitzar_particles::ParticleBuffer particles(LatticeParticleCount);
    blitzar_particles::ParticleAccelerationBuffer direct_force(LatticeParticleCount);
    blitzar_particles::ParticleAccelerationBuffer bh_force(LatticeParticleCount);
    blitzar_particles::ParticleAccelerationBuffer fmm_force(LatticeParticleCount);
    blitzar_particles::ParticleAccelerationBuffer kifmm_force(LatticeParticleCount);

    FillParticles(particles, false);

    const SolverCase config{
        particles.State(), gravity, LatticeParticleCount, DefaultOpeningAngle, OpenLeafCapacity};

    blitzar_direct::DirectSolver direct(gravity, LatticeParticleCount);

    if (EvaluatePeriodic(direct, particles.State(), direct_force.View(), &periodic) !=
        BLITZAR_STATUS_OK) {
        return false;
    }

    if (!EvaluateSolverCase<blitzar_barnes_hut::BarnesHutSettings, blitzar_barnes_hut::BhSolver>(
            config, bh_force.View(), &periodic) ||
        ForceRelativeError(direct_force.View(), bh_force.View()) > ApproximateRelativeTolerance) {
        return false;
    }

    if (!EvaluateSolverCase<blitzar_fmm::FmmSettings, blitzar_fmm::FmmSolver>(
            config, fmm_force.View(), &periodic) ||
        ForceRelativeError(direct_force.View(), fmm_force.View()) > ApproximateRelativeTolerance) {
        return false;
    }

    if (!EvaluateSolverCase<blitzar_kifmm::KifmmSettings, blitzar_kifmm::KifmmSolver>(
            config, kifmm_force.View(), &periodic) ||
        ForceRelativeError(direct_force.View(), kifmm_force.View()) >
            ApproximateRelativeTolerance) {
        return false;
    }

    return true;
}

[[nodiscard]] bool RunSeamNoDoubleCountCase() noexcept
{
    constexpr std::size_t SeamParticleCount = 2;
    const blitzar_physics::PeriodicDomain periodic{true, BoxOrigin, BoxSize};
    const blitzar_physics::GravityParameters gravity{1.0, ExactSoftening};
    blitzar_particles::ParticleBuffer particles(SeamParticleCount);
    blitzar_particles::ParticleAccelerationBuffer periodic_force(SeamParticleCount);
    blitzar_particles::ParticleAccelerationBuffer expected_force(SeamParticleCount);
    blitzar_particles::ParticleAccelerationBuffer open_force(SeamParticleCount);
    blitzar_particles::ParticleAccelerationBuffer bh_force(SeamParticleCount);

    (void)particles.SetPosition(0, {-0.99, 0.0, 0.0});
    (void)particles.SetPosition(1, {0.99, 0.0, 0.0});
    (void)particles.SetVelocity(0, {});
    (void)particles.SetVelocity(1, {});
    (void)particles.SetMass(0, 1.0);
    (void)particles.SetMass(1, 1.0);

    const SolverCase config{
        particles.State(), gravity, SeamParticleCount, ExactOpeningAngle, SeamParticleCount};

    blitzar_direct::DirectSolver direct(gravity, SeamParticleCount);

    if (EvaluatePeriodic(direct, particles.State(), periodic_force.View(), &periodic) !=
            BLITZAR_STATUS_OK ||
        EvaluatePeriodic(direct, particles.State(), open_force.View(), nullptr) !=
            BLITZAR_STATUS_OK) {
        return false;
    }

    const double softened = 0.02 * 0.02 + ExactSoftening * ExactSoftening;
    const double factor = 1.0 / (softened * std::sqrt(softened));
    const blitzar_core::ForceView expected = expected_force.View();

    expected.x[0] = -factor * 0.02;
    expected.y[0] = 0.0;
    expected.z[0] = 0.0;
    expected.x[1] = factor * 0.02;
    expected.y[1] = 0.0;
    expected.z[1] = 0.0;

    if (ForceRelativeError(periodic_force.View(), expected_force.View()) > 1.0e-9 ||
        ForceRelativeError(open_force.View(), expected_force.View()) < 0.5 ||
        !EvaluateSolverCase<blitzar_barnes_hut::BarnesHutSettings, blitzar_barnes_hut::BhSolver>(
            config, bh_force.View(), &periodic) ||
        ForceRelativeError(periodic_force.View(), bh_force.View()) != 0.0) {
        return false;
    }

    return true;
}

[[nodiscard]] bool RunOpenBoundaryUnchangedCase() noexcept
{
    const blitzar_physics::PeriodicDomain disabled{};
    const blitzar_physics::GravityParameters gravity{1.0, ApproximateSoftening};
    blitzar_particles::ParticleBuffer particles(LatticeParticleCount);
    blitzar_particles::ParticleAccelerationBuffer untouched(LatticeParticleCount);
    blitzar_particles::ParticleAccelerationBuffer opened(LatticeParticleCount);

    FillParticles(particles, false);

    const SolverCase config{
        particles.State(), gravity, LatticeParticleCount, DefaultOpeningAngle, OpenLeafCapacity};

    blitzar_direct::DirectSolver direct(gravity, LatticeParticleCount);

    if (EvaluatePeriodic(direct, particles.State(), untouched.View(), &disabled) !=
            BLITZAR_STATUS_OK ||
        EvaluatePeriodic(direct, particles.State(), opened.View(), nullptr) != BLITZAR_STATUS_OK ||
        ForceRelativeError(untouched.View(), opened.View()) != 0.0) {
        return false;
    }

    if (!EvaluateSolverCase<blitzar_barnes_hut::BarnesHutSettings, blitzar_barnes_hut::BhSolver>(
            config, untouched.View(), &disabled) ||
        !EvaluateSolverCase<blitzar_barnes_hut::BarnesHutSettings, blitzar_barnes_hut::BhSolver>(
            config, opened.View(), nullptr) ||
        ForceRelativeError(untouched.View(), opened.View()) != 0.0) {
        return false;
    }

    if (!EvaluateSolverCase<blitzar_fmm::FmmSettings, blitzar_fmm::FmmSolver>(
            config, untouched.View(), &disabled) ||
        !EvaluateSolverCase<blitzar_fmm::FmmSettings, blitzar_fmm::FmmSolver>(
            config, opened.View(), nullptr) ||
        ForceRelativeError(untouched.View(), opened.View()) != 0.0) {
        return false;
    }

    if (!EvaluateSolverCase<blitzar_kifmm::KifmmSettings, blitzar_kifmm::KifmmSolver>(
            config, untouched.View(), &disabled) ||
        !EvaluateSolverCase<blitzar_kifmm::KifmmSettings, blitzar_kifmm::KifmmSolver>(
            config, opened.View(), nullptr) ||
        ForceRelativeError(untouched.View(), opened.View()) != 0.0) {
        return false;
    }

    return true;
}

[[nodiscard]] bool RunRefitParityCase() noexcept
{
    const blitzar_physics::PeriodicDomain periodic{true, BoxOrigin, BoxSize};
    const blitzar_physics::GravityParameters gravity{1.0, ApproximateSoftening};
    blitzar_particles::ParticleBuffer particles(LatticeParticleCount);
    blitzar_particles::ParticleAccelerationBuffer first_force(LatticeParticleCount);
    blitzar_particles::ParticleAccelerationBuffer second_force(LatticeParticleCount);

    FillParticles(particles, false);

    const SolverCase config{
        particles.State(), gravity, LatticeParticleCount, DefaultOpeningAngle, OpenLeafCapacity};

    if (!EvaluateSolverCase<blitzar_barnes_hut::BarnesHutSettings, blitzar_barnes_hut::BhSolver>(
            config, first_force.View(), &periodic) ||
        !EvaluateSolverCase<blitzar_barnes_hut::BarnesHutSettings, blitzar_barnes_hut::BhSolver>(
            config, second_force.View(), &periodic) ||
        ForceRelativeError(first_force.View(), second_force.View()) != 0.0) {
        return false;
    }

    if (!EvaluateSolverCase<blitzar_fmm::FmmSettings, blitzar_fmm::FmmSolver>(
            config, first_force.View(), &periodic) ||
        !EvaluateSolverCase<blitzar_fmm::FmmSettings, blitzar_fmm::FmmSolver>(
            config, second_force.View(), &periodic) ||
        ForceRelativeError(first_force.View(), second_force.View()) != 0.0) {
        return false;
    }

    if (!EvaluateSolverCase<blitzar_kifmm::KifmmSettings, blitzar_kifmm::KifmmSolver>(
            config, first_force.View(), &periodic) ||
        !EvaluateSolverCase<blitzar_kifmm::KifmmSettings, blitzar_kifmm::KifmmSolver>(
            config, second_force.View(), &periodic) ||
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
