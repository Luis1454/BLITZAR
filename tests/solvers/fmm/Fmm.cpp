#include "core/contracts/Execution.hpp"
#include "fixtures/AllocationMonitor.hpp"
#include "fixtures/Check.hpp"
#include "particles/buffers/AccelerationBuffer.hpp"
#include "particles/buffers/ParticleBuffer.hpp"
#include "solvers/direct/DirectSolver.hpp"
#include "solvers/fmm/FmmSolver.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdio>

namespace {

[[nodiscard]] blitzar_fmm::FmmSettings MakeSettings(
    std::size_t particle_count, double opening_angle) noexcept
{
    return {opening_angle, particle_count, particle_count * 8 + 1, 4, 16};
}

void FillParticles(blitzar_particles::ParticleBuffer& particles) noexcept
{
    for (std::size_t index = 0; index < particles.Count(); ++index) {
        const std::size_t column = index % 16;
        const std::size_t row = (index / 16) % 16;
        const std::size_t layer = index / 256;
        const blitzar_core::Vector3 position{0.25 * static_cast<double>(column) - 2.0,
            0.25 * static_cast<double>(row) - 2.0, 0.25 * static_cast<double>(layer) - 0.5};

        (void)particles.SetPosition(index, position);
        (void)particles.SetVelocity(index, {});
        (void)particles.SetMass(index, 1.0 + 0.25 * static_cast<double>(index % 5));
    }
}

[[nodiscard]] bool RunDirectEquivalentCase() noexcept
{
    constexpr std::size_t ParticleCount = 32;
    const blitzar_physics::GravityParameters gravity{1.0, 0.1};
    blitzar_particles::ParticleBuffer particles(ParticleCount);
    blitzar_particles::AccelerationBuffer direct_force(ParticleCount);
    blitzar_particles::AccelerationBuffer fmm_force(ParticleCount);

    FillParticles(particles);

    blitzar_direct::DirectSolver direct(gravity, ParticleCount);
    blitzar_fmm::FmmSolver fmm(gravity, MakeSettings(ParticleCount, 0.0), ParticleCount);
    const blitzar_core::ExecutionSettings execution{};

    if (direct.Compute(particles.State(), direct_force.View(), execution) != BLITZAR_STATUS_OK ||
        fmm.Compute(particles.State(), fmm_force.View(), execution) != BLITZAR_STATUS_OK ||
        fmm.Kind() != blitzar_solvers::SolverKind::Fmm || fmm.BuildCount() != 1) {
        return false;
    }

    const blitzar_core::ForceView expected = direct_force.View();
    const blitzar_core::ForceView actual = fmm_force.View();

    for (std::size_t index = 0; index < ParticleCount; ++index) {
        if (expected.x[index] != actual.x[index] || expected.y[index] != actual.y[index] ||
            expected.z[index] != actual.z[index]) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] bool RunApproximationCase() noexcept
{
    constexpr std::size_t ParticleCount = 256;
    const blitzar_physics::GravityParameters gravity{1.0, 0.1};
    blitzar_particles::ParticleBuffer particles(ParticleCount);
    blitzar_particles::AccelerationBuffer direct_force(ParticleCount);
    blitzar_particles::AccelerationBuffer fmm_force(ParticleCount);

    FillParticles(particles);

    blitzar_direct::DirectSolver direct(gravity, ParticleCount);
    blitzar_fmm::FmmSolver fmm(gravity, MakeSettings(ParticleCount, 0.35), ParticleCount);
    const blitzar_core::ExecutionSettings execution{};

    if (direct.Compute(particles.State(), direct_force.View(), execution) != BLITZAR_STATUS_OK ||
        fmm.Compute(particles.State(), fmm_force.View(), execution) != BLITZAR_STATUS_OK) {
        return false;
    }

    const blitzar_core::ForceView expected = direct_force.View();
    const blitzar_core::ForceView actual = fmm_force.View();
    double maximum_relative_error = 0.0;

    for (std::size_t index = 0; index < ParticleCount; ++index) {
        const double expected_norm = std::sqrt(expected.x[index] * expected.x[index] +
                                               expected.y[index] * expected.y[index] +
                                               expected.z[index] * expected.z[index]);

        const double error_norm = std::sqrt(
            (expected.x[index] - actual.x[index]) * (expected.x[index] - actual.x[index]) +
            (expected.y[index] - actual.y[index]) * (expected.y[index] - actual.y[index]) +
            (expected.z[index] - actual.z[index]) * (expected.z[index] - actual.z[index]));

        maximum_relative_error =
            std::max(maximum_relative_error, error_norm / std::max(1.0, expected_norm));
    }

    return maximum_relative_error < 0.05;
}

[[nodiscard]] bool RunTransactionalFailureCase() noexcept
{
    blitzar_particles::ParticleBuffer particles(3);
    blitzar_particles::AccelerationBuffer forces(3);

    FillParticles(particles);

    (void)particles.SetPosition(1, {0.0, 0.0, 0.0});
    (void)particles.SetPosition(2, {0.0, 0.0, 0.0});

    const blitzar_core::ForceView view = forces.View();

    for (std::size_t index = 0; index < 3; ++index) {
        view.x[index] = 4.0;
        view.y[index] = 5.0;
        view.z[index] = 6.0;
    }

    blitzar_fmm::FmmSolver fmm({1.0, 0.0}, MakeSettings(3, 0.0), 3);

    const blitzar_status status =
        fmm.Compute(particles.State(), view, blitzar_core::ExecutionSettings{});

    if (status != BLITZAR_STATUS_SINGULARITY) {
        return false;
    }

    for (std::size_t index = 0; index < 3; ++index) {
        if (view.x[index] != 4.0 || view.y[index] != 5.0 || view.z[index] != 6.0) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] bool RunLargeCapacityCase() noexcept
{
    constexpr std::size_t ParticleCount = 512;
    const blitzar_physics::GravityParameters gravity{1.0, 0.1};
    blitzar_particles::ParticleBuffer particles(ParticleCount);
    blitzar_particles::AccelerationBuffer forces(ParticleCount);

    FillParticles(particles);

    blitzar_fmm::FmmSolver fmm(gravity, MakeSettings(ParticleCount, 0.35), ParticleCount);
    const blitzar_core::ExecutionSettings execution{};
    const auto first_start = std::chrono::steady_clock::now();
    const blitzar_status first_status = fmm.Compute(particles.State(), forces.View(), execution);
    const auto first_end = std::chrono::steady_clock::now();

    if (first_status != BLITZAR_STATUS_OK || fmm.BuildCount() != 1) {
        return false;
    }

    blitzar_tests::BeginAllocationCounting();

    const auto second_start = std::chrono::steady_clock::now();
    const blitzar_status second_status = fmm.Compute(particles.State(), forces.View(), execution);
    const auto second_end = std::chrono::steady_clock::now();
    const std::size_t allocations = blitzar_tests::EndAllocationCounting();

    if (second_status != BLITZAR_STATUS_OK || fmm.RefitCount() != 1 || allocations != 0) {
        return false;
    }

    const auto first_time =
        std::chrono::duration_cast<std::chrono::microseconds>(first_end - first_start);

    const auto second_time =
        std::chrono::duration_cast<std::chrono::microseconds>(second_end - second_start);

    std::printf("fmm-large-n=%zu build-us=%lld refit-us=%lld\n", ParticleCount,
        static_cast<long long>(first_time.count()), static_cast<long long>(second_time.count()));

    return true;
}

} // namespace

int main()
{
    BLITZAR_CHECK(RunDirectEquivalentCase());
    BLITZAR_CHECK(RunApproximationCase());
    BLITZAR_CHECK(RunTransactionalFailureCase());
    BLITZAR_CHECK(RunLargeCapacityCase());

    return 0;
}
