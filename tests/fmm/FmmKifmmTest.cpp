#include "core/CoreExecution.hpp"
#include "fixtures/FixtureAllocationMonitor.hpp"
#include "fixtures/FixtureCheck.hpp"
#include "fixtures/FixtureForce.hpp"
#include "particles/buffer/ParticleAccelerationBuffer.hpp"
#include "particles/buffer/ParticleBuffer.hpp"
#include "solvers/SolverForceRequest.hpp"
#include "solvers/direct/DirectSolver.hpp"
#include "solvers/fmm/kifmm/KifmmSolver.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace {

[[nodiscard]] blitzar_kifmm::KifmmSettings MakeSettings(
    std::size_t particle_count, double opening_angle) noexcept
{
    return {opening_angle, particle_count, particle_count * 8 + 1, 4, 16};
}

[[nodiscard]] blitzar_solvers::SolverTreeResources MakeResources(
    blitzar_kifmm::KifmmSettings settings, std::size_t local_capacity)
{
    return {{local_capacity, settings.max_cells, settings.leaf_capacity, settings.max_depth},
        {settings.max_particles, settings.max_cells, settings.leaf_capacity, settings.max_depth}};
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

[[nodiscard]] double MaximumRelativeError(
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

[[nodiscard]] bool RunExactParity() noexcept
{
    constexpr std::size_t particle_count = 32;
    blitzar_particles::ParticleBuffer particles(particle_count);
    blitzar_particles::ParticleAccelerationBuffer direct_force(particle_count);
    blitzar_particles::ParticleAccelerationBuffer kifmm_force(particle_count);
    const auto settings = MakeSettings(particle_count, 0.0);
    auto resources = MakeResources(settings, particle_count);
    blitzar_direct::DirectSolver direct({1.0, 0.1}, particle_count);
    blitzar_kifmm::KifmmSolver kifmm({1.0, 0.1}, settings, particle_count, resources);
    const blitzar_core::ExecutionSettings execution{};

    FillParticles(particles);

    if (blitzar_tests::EvaluateLocal(direct, particles.State(), direct_force.View(), execution) !=
            BLITZAR_STATUS_OK ||
        blitzar_tests::EvaluateLocal(kifmm, particles.State(), kifmm_force.View(), execution) !=
            BLITZAR_STATUS_OK ||
        kifmm.Kind() != blitzar_solvers::SolverKind::Kifmm || kifmm.BuildCount() != 1) {
        return false;
    }

    return MaximumRelativeError(direct_force.View(), kifmm_force.View()) < 1.0e-12;
}

[[nodiscard]] bool RunApproximation() noexcept
{
    constexpr std::size_t particle_count = 256;
    blitzar_particles::ParticleBuffer particles(particle_count);
    blitzar_particles::ParticleAccelerationBuffer direct_force(particle_count);
    blitzar_particles::ParticleAccelerationBuffer kifmm_force(particle_count);
    const auto settings = MakeSettings(particle_count, 0.35);
    auto resources = MakeResources(settings, particle_count);
    blitzar_direct::DirectSolver direct({1.0, 0.1}, particle_count);
    blitzar_kifmm::KifmmSolver kifmm({1.0, 0.1}, settings, particle_count, resources);
    const blitzar_core::ExecutionSettings execution{};

    FillParticles(particles);

    if (blitzar_tests::EvaluateLocal(direct, particles.State(), direct_force.View(), execution) !=
            BLITZAR_STATUS_OK ||
        blitzar_tests::EvaluateLocal(kifmm, particles.State(), kifmm_force.View(), execution) !=
            BLITZAR_STATUS_OK) {
        return false;
    }

    return MaximumRelativeError(direct_force.View(), kifmm_force.View()) < 0.12;
}

[[nodiscard]] bool RunRepeatabilityAndAllocation() noexcept
{
    constexpr std::size_t particle_count = 128;
    blitzar_particles::ParticleBuffer particles(particle_count);
    blitzar_particles::ParticleAccelerationBuffer first_force(particle_count);
    blitzar_particles::ParticleAccelerationBuffer second_force(particle_count);
    const auto settings = MakeSettings(particle_count, 0.35);
    auto resources = MakeResources(settings, particle_count);
    blitzar_kifmm::KifmmSolver kifmm({1.0, 0.1}, settings, particle_count, resources);
    const blitzar_core::ExecutionSettings execution{};

    FillParticles(particles);

    if (blitzar_tests::EvaluateLocal(kifmm, particles.State(), first_force.View(), execution) !=
        BLITZAR_STATUS_OK) {
        return false;
    }

    blitzar_tests::BeginAllocationCounting();

    const blitzar_status status =
        blitzar_tests::EvaluateLocal(kifmm, particles.State(), second_force.View(), execution);

    const std::size_t allocations = blitzar_tests::EndAllocationCounting();

    return status == BLITZAR_STATUS_OK && allocations == 0 &&
           MaximumRelativeError(first_force.View(), second_force.View()) == 0.0 &&
           kifmm.RefitCount() == 1;
}

[[nodiscard]] bool RunSingularityRollback() noexcept
{
    blitzar_particles::ParticleBuffer particles(3);
    blitzar_particles::ParticleAccelerationBuffer forces(3);
    const auto settings = MakeSettings(3, 0.0);
    auto resources = MakeResources(settings, 3);
    blitzar_kifmm::KifmmSolver kifmm({1.0, 0.0}, settings, 3, resources);
    const blitzar_core::ForceView view = forces.View();

    for (std::size_t index = 0; index < 3; ++index) {
        (void)particles.SetPosition(index, {0.0, 0.0, 0.0});
        (void)particles.SetVelocity(index, {});
        (void)particles.SetMass(index, 1.0);

        view.x[index] = 4.0;
        view.y[index] = 5.0;
        view.z[index] = 6.0;
    }

    const blitzar_status status = blitzar_tests::EvaluateLocal(
        kifmm, particles.State(), view, blitzar_core::ExecutionSettings{});

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

[[nodiscard]] bool RunEmptyAndDegenerate() noexcept
{
    constexpr std::size_t particle_count = 4;
    const auto settings = MakeSettings(particle_count, 0.0);
    auto resources = MakeResources(settings, particle_count);
    blitzar_kifmm::KifmmSolver kifmm({1.0, 0.1}, settings, particle_count, resources);
    const blitzar_core::ExecutionSettings execution{};

    blitzar_particles::ParticleBuffer empty_particles(0);
    blitzar_particles::ParticleAccelerationBuffer empty_forces(0);

    if (blitzar_tests::EvaluateLocal(
            kifmm, empty_particles.State(), empty_forces.View(), execution) != BLITZAR_STATUS_OK) {
        return false;
    }

    blitzar_particles::ParticleBuffer particles(particle_count);
    blitzar_particles::ParticleAccelerationBuffer forces(particle_count);

    for (std::size_t index = 0; index < particle_count; ++index) {
        (void)particles.SetPosition(index, {});
        (void)particles.SetVelocity(index, {});
        (void)particles.SetMass(index, 1.0);
    }

    if (blitzar_tests::EvaluateLocal(kifmm, particles.State(), forces.View(), execution) !=
        BLITZAR_STATUS_OK) {
        return false;
    }

    const blitzar_core::ForceView view = forces.View();

    for (std::size_t index = 0; index < particle_count; ++index) {
        if (!std::isfinite(view.x[index]) || !std::isfinite(view.y[index]) ||
            !std::isfinite(view.z[index]) || view.x[index] != 0.0 || view.y[index] != 0.0 ||
            view.z[index] != 0.0) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] bool RunCapacityAndGeneration() noexcept
{
    constexpr std::size_t particle_count = 8;
    blitzar_particles::ParticleBuffer particles(particle_count);
    blitzar_particles::ParticleAccelerationBuffer forces(particle_count);
    const auto settings = MakeSettings(particle_count, 0.35);
    auto resources = MakeResources(settings, particle_count);
    blitzar_kifmm::KifmmSolver kifmm({1.0, 0.1}, settings, particle_count, resources);
    const blitzar_core::ExecutionSettings execution{};

    FillParticles(particles);

    if (kifmm.Prepare(particle_count + 1) != BLITZAR_STATUS_INVALID_ARGUMENT ||
        resources.Local().Prepare(particles.State()) != BLITZAR_STATUS_OK) {
        return false;
    }

    const blitzar_trees::OctreeView tree = resources.Local().View();
    const std::uint64_t generation = tree.Generation();
    const blitzar_solvers::SolverForceRequest::Tree request{particles.State(), particles.State(),
        forces.View(), execution, resources.Local(), tree,
        blitzar_solvers::SolverForceSourceKind::Local, false, true};

    return kifmm.Evaluate(request) == BLITZAR_STATUS_OK &&
           resources.Local().View().Generation() == generation;
}

} // namespace

int main()
{
    BLITZAR_CHECK(RunExactParity());
    BLITZAR_CHECK(RunApproximation());
    BLITZAR_CHECK(RunRepeatabilityAndAllocation());
    BLITZAR_CHECK(RunSingularityRollback());
    BLITZAR_CHECK(RunEmptyAndDegenerate());
    BLITZAR_CHECK(RunCapacityAndGeneration());

    return 0;
}
