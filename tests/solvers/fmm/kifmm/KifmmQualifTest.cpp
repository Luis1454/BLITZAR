#include "core/CoreExecution.hpp"
#include "fixtures/FixtureAllocationMonitor.hpp"
#include "fixtures/FixtureCheck.hpp"
#include "fixtures/FixtureForce.hpp"
#include "integration/kdk/KdkLeapfrog.hpp"
#include "particles/buffer/ParticleAccelerationBuffer.hpp"
#include "particles/buffer/ParticleBuffer.hpp"
#include "physics/conservation/ConservationMetrics.hpp"
#include "solvers/direct/DirectSolver.hpp"
#include "solvers/fmm/FmmSolver.hpp"
#include "solvers/fmm/kifmm/KifmmSolver.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace {

constexpr std::size_t SnapshotCount = 3;

struct ParticleSnapshot final {
    std::array<double, SnapshotCount> x{};
    std::array<double, SnapshotCount> y{};
    std::array<double, SnapshotCount> z{};
    std::array<double, SnapshotCount> velocity_x{};
    std::array<double, SnapshotCount> velocity_y{};
    std::array<double, SnapshotCount> velocity_z{};
    std::array<double, SnapshotCount> mass{};
};

struct ForceSnapshot final {
    std::array<double, SnapshotCount> x{};
    std::array<double, SnapshotCount> y{};
    std::array<double, SnapshotCount> z{};
};

[[nodiscard]] bool SameScalar(double first, double second) noexcept
{
    return first == second || (std::isnan(first) && std::isnan(second));
}

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
    if (expected.count != actual.count) {
        return std::numeric_limits<double>::infinity();
    }

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

[[nodiscard]] bool SameForces(
    blitzar_core::ForceView first, blitzar_core::ForceView second) noexcept
{
    if (first.count != second.count) {
        return false;
    }

    for (std::size_t index = 0; index < first.count; ++index) {
        if (first.x[index] != second.x[index] || first.y[index] != second.y[index] ||
            first.z[index] != second.z[index]) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] bool SameParticleState(
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

[[nodiscard]] ParticleSnapshot CaptureState(blitzar_core::ParticleStateView state) noexcept
{
    ParticleSnapshot snapshot{};

    for (std::size_t index = 0; index < SnapshotCount; ++index) {
        snapshot.x[index] = state.x[index];
        snapshot.y[index] = state.y[index];
        snapshot.z[index] = state.z[index];
        snapshot.velocity_x[index] = state.velocity_x[index];
        snapshot.velocity_y[index] = state.velocity_y[index];
        snapshot.velocity_z[index] = state.velocity_z[index];
        snapshot.mass[index] = state.mass[index];
    }

    return snapshot;
}

[[nodiscard]] bool SameParticleState(
    blitzar_core::ParticleStateView state, const ParticleSnapshot& snapshot) noexcept
{
    if (state.count != SnapshotCount) {
        return false;
    }

    for (std::size_t index = 0; index < SnapshotCount; ++index) {
        if (!SameScalar(state.x[index], snapshot.x[index]) ||
            !SameScalar(state.y[index], snapshot.y[index]) ||
            !SameScalar(state.z[index], snapshot.z[index]) ||
            !SameScalar(state.velocity_x[index], snapshot.velocity_x[index]) ||
            !SameScalar(state.velocity_y[index], snapshot.velocity_y[index]) ||
            !SameScalar(state.velocity_z[index], snapshot.velocity_z[index]) ||
            !SameScalar(state.mass[index], snapshot.mass[index])) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] ForceSnapshot CaptureForces(blitzar_core::ForceView forces) noexcept
{
    ForceSnapshot snapshot{};

    for (std::size_t index = 0; index < SnapshotCount; ++index) {
        snapshot.x[index] = forces.x[index];
        snapshot.y[index] = forces.y[index];
        snapshot.z[index] = forces.z[index];
    }

    return snapshot;
}

[[nodiscard]] bool SameForces(
    blitzar_core::ForceView forces, const ForceSnapshot& snapshot) noexcept
{
    if (forces.count != SnapshotCount) {
        return false;
    }

    for (std::size_t index = 0; index < SnapshotCount; ++index) {
        if (forces.x[index] != snapshot.x[index] || forces.y[index] != snapshot.y[index] ||
            forces.z[index] != snapshot.z[index]) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] bool RunOracleComparison() noexcept
{
    constexpr std::size_t exact_count = 24;
    constexpr std::size_t approximate_count = 256;
    const blitzar_physics::GravityParameters gravity{1.0, 0.1};
    const blitzar_core::ExecutionSettings execution{};

    blitzar_particles::ParticleBuffer exact_particles(exact_count);
    blitzar_particles::ParticleAccelerationBuffer exact_direct_force(exact_count);
    blitzar_particles::ParticleAccelerationBuffer exact_fmm_force(exact_count);
    blitzar_particles::ParticleAccelerationBuffer exact_kifmm_force(exact_count);
    const auto exact_settings = MakeSettings(exact_count, 0.0);
    auto exact_resources = MakeResources(exact_settings, exact_count);
    auto exact_fmm_resources = MakeResources(exact_settings, exact_count);
    blitzar_direct::DirectSolver exact_direct(gravity, exact_count);
    blitzar_fmm::FmmSolver exact_fmm(gravity,
        blitzar_fmm::FmmSettings{exact_settings.opening_angle, exact_settings.max_particles,
            exact_settings.max_cells, exact_settings.leaf_capacity, exact_settings.max_depth},
        exact_count, exact_fmm_resources);

    blitzar_kifmm::KifmmSolver exact_kifmm(gravity, exact_settings, exact_count, exact_resources);

    FillParticles(exact_particles);

    if (blitzar_tests::EvaluateLocal(exact_direct, exact_particles.State(),
            exact_direct_force.View(), execution) != BLITZAR_STATUS_OK ||
        blitzar_tests::EvaluateLocal(exact_fmm, exact_particles.State(), exact_fmm_force.View(),
            execution) != BLITZAR_STATUS_OK ||
        blitzar_tests::EvaluateLocal(exact_kifmm, exact_particles.State(), exact_kifmm_force.View(),
            execution) != BLITZAR_STATUS_OK ||
        MaximumRelativeError(exact_direct_force.View(), exact_fmm_force.View()) >= 1.0e-12 ||
        MaximumRelativeError(exact_direct_force.View(), exact_kifmm_force.View()) >= 1.0e-12) {
        return false;
    }

    blitzar_particles::ParticleBuffer approximate_particles(approximate_count);
    blitzar_particles::ParticleAccelerationBuffer approximate_direct_force(approximate_count);
    blitzar_particles::ParticleAccelerationBuffer approximate_fmm_force(approximate_count);
    blitzar_particles::ParticleAccelerationBuffer approximate_kifmm_force(approximate_count);
    const auto approximate_settings = MakeSettings(approximate_count, 0.35);
    auto approximate_fmm_resources = MakeResources(approximate_settings, approximate_count);
    auto approximate_kifmm_resources = MakeResources(approximate_settings, approximate_count);
    blitzar_direct::DirectSolver approximate_direct(gravity, approximate_count);
    blitzar_fmm::FmmSolver approximate_fmm(gravity,
        blitzar_fmm::FmmSettings{approximate_settings.opening_angle,
            approximate_settings.max_particles, approximate_settings.max_cells,
            approximate_settings.leaf_capacity, approximate_settings.max_depth},
        approximate_count, approximate_fmm_resources);

    blitzar_kifmm::KifmmSolver approximate_kifmm(
        gravity, approximate_settings, approximate_count, approximate_kifmm_resources);

    FillParticles(approximate_particles);

    if (blitzar_tests::EvaluateLocal(approximate_direct, approximate_particles.State(),
            approximate_direct_force.View(), execution) != BLITZAR_STATUS_OK ||
        blitzar_tests::EvaluateLocal(approximate_fmm, approximate_particles.State(),
            approximate_fmm_force.View(), execution) != BLITZAR_STATUS_OK ||
        blitzar_tests::EvaluateLocal(approximate_kifmm, approximate_particles.State(),
            approximate_kifmm_force.View(), execution) != BLITZAR_STATUS_OK) {
        return false;
    }

    return MaximumRelativeError(approximate_direct_force.View(), approximate_fmm_force.View()) <
               0.05 &&
           MaximumRelativeError(approximate_direct_force.View(), approximate_kifmm_force.View()) <
               0.12;
}

[[nodiscard]] bool RunOrderingAndRepeatability() noexcept
{
    constexpr std::size_t particle_count = 128;
    const auto settings = MakeSettings(particle_count, 0.35);
    blitzar_particles::ParticleBuffer particles(particle_count);
    blitzar_particles::ParticleAccelerationBuffer fmm_force(particle_count);
    blitzar_particles::ParticleAccelerationBuffer first_kifmm_force(particle_count);
    blitzar_particles::ParticleAccelerationBuffer second_kifmm_force(particle_count);
    auto fmm_resources = MakeResources(settings, particle_count);
    auto kifmm_resources = MakeResources(settings, particle_count);
    blitzar_fmm::FmmSolver fmm({1.0, 0.1},
        blitzar_fmm::FmmSettings{settings.opening_angle, settings.max_particles, settings.max_cells,
            settings.leaf_capacity, settings.max_depth},
        particle_count, fmm_resources);

    blitzar_kifmm::KifmmSolver kifmm({1.0, 0.1}, settings, particle_count, kifmm_resources);
    const blitzar_core::ExecutionSettings execution{};

    FillParticles(particles);

    if (blitzar_tests::EvaluateLocal(fmm, particles.State(), fmm_force.View(), execution) !=
            BLITZAR_STATUS_OK ||
        blitzar_tests::EvaluateLocal(
            kifmm, particles.State(), first_kifmm_force.View(), execution) != BLITZAR_STATUS_OK) {
        return false;
    }

    const blitzar_trees::OctreeView fmm_tree = fmm_resources.Local().View();
    const blitzar_trees::OctreeView kifmm_tree = kifmm_resources.Local().View();

    if (fmm_tree.Indices().size() != kifmm_tree.Indices().size()) {
        return false;
    }

    for (std::size_t index = 0; index < fmm_tree.Indices().size(); ++index) {
        if (fmm_tree.Indices()[index] != kifmm_tree.Indices()[index]) {
            return false;
        }
    }

    const std::uint64_t first_generation = kifmm_tree.Generation();

    blitzar_tests::BeginAllocationCounting();

    const blitzar_status second_status = blitzar_tests::EvaluateLocal(
        kifmm, particles.State(), second_kifmm_force.View(), execution);

    const std::size_t allocations = blitzar_tests::EndAllocationCounting();

    const blitzar_trees::OctreeView repeated_tree = kifmm_resources.Local().View();

    if (second_status != BLITZAR_STATUS_OK || allocations != 0 ||
        !SameForces(first_kifmm_force.View(), second_kifmm_force.View()) ||
        kifmm.RefitCount() != 1 || repeated_tree.Generation() == first_generation ||
        repeated_tree.Indices().size() != kifmm_tree.Indices().size()) {
        return false;
    }

    for (std::size_t index = 0; index < repeated_tree.Indices().size(); ++index) {
        if (repeated_tree.Indices()[index] != kifmm_tree.Indices()[index]) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] bool InitializeConservationState(
    blitzar_particles::ParticleBuffer& particles, double circular_speed) noexcept
{
    return particles.SetPosition(0, {-0.5, 0.0, 0.0}) == BLITZAR_STATUS_OK &&
           particles.SetPosition(1, {0.5, 0.0, 0.0}) == BLITZAR_STATUS_OK &&
           particles.SetVelocity(0, {0.0, circular_speed, 0.0}) == BLITZAR_STATUS_OK &&
           particles.SetVelocity(1, {0.0, -circular_speed, 0.0}) == BLITZAR_STATUS_OK &&
           particles.SetMass(0, 1.0) == BLITZAR_STATUS_OK &&
           particles.SetMass(1, 1.0) == BLITZAR_STATUS_OK;
}

[[nodiscard]] bool RunConservationCase() noexcept
{
    constexpr std::size_t particle_count = 2;
    constexpr double softening = 0.05;
    constexpr double timestep = 0.001;
    constexpr std::size_t steps = 1024;
    const double circular_speed = std::sqrt(0.5 / std::pow(1.0 + softening * softening, 1.5));
    const blitzar_physics::GravityParameters gravity{1.0, softening};
    blitzar_particles::ParticleBuffer direct_particles(particle_count);
    blitzar_particles::ParticleBuffer kifmm_particles(particle_count);

    if (!InitializeConservationState(direct_particles, circular_speed) ||
        !InitializeConservationState(kifmm_particles, circular_speed)) {
        return false;
    }

    blitzar_particles::ParticleAccelerationBuffer direct_accelerations(particle_count);
    blitzar_particles::ParticleAccelerationBuffer kifmm_accelerations(particle_count);
    blitzar_integration::KdkCheckpoint direct_checkpoint(particle_count);
    blitzar_integration::KdkCheckpoint kifmm_checkpoint(particle_count);
    blitzar_direct::DirectSolver direct(gravity, particle_count);
    const auto settings = MakeSettings(particle_count, 0.0);
    auto resources = MakeResources(settings, particle_count);
    blitzar_kifmm::KifmmSolver kifmm(gravity, settings, particle_count, resources);
    blitzar_solvers::SolverCpuForceProvider<blitzar_direct::DirectSolver> direct_provider(direct);
    blitzar_solvers::SolverCpuForceProvider<blitzar_kifmm::KifmmSolver> kifmm_provider(kifmm);
    const blitzar_core::ExecutionSettings execution{};
    blitzar_integration::KdkLeapfrog integrator;

    if (direct.Prepare(particle_count) != BLITZAR_STATUS_OK ||
        kifmm.Prepare(particle_count) != BLITZAR_STATUS_OK) {
        return false;
    }

    blitzar_integration_kdk::AdvanceState<decltype(direct_provider)> direct_state{direct_particles,
        direct_accelerations, direct_checkpoint, direct_provider, timestep, execution,
        direct_particles.State()};

    blitzar_integration_kdk::AdvanceState<decltype(kifmm_provider)> kifmm_state{kifmm_particles,
        kifmm_accelerations, kifmm_checkpoint, kifmm_provider, timestep, execution,
        kifmm_particles.State()};

    blitzar_physics::ConservationMetrics direct_initial{};
    blitzar_physics::ConservationMetrics kifmm_initial{};

    if (blitzar_physics::ComputeConservationMetrics(
            direct_particles.State(), gravity, direct_initial) != BLITZAR_STATUS_OK ||
        blitzar_physics::ComputeConservationMetrics(
            kifmm_particles.State(), gravity, kifmm_initial) != BLITZAR_STATUS_OK) {
        return false;
    }

    for (std::size_t step = 0; step < steps; ++step) {
        if (integrator.Advance(direct_state) != BLITZAR_STATUS_OK ||
            integrator.Advance(kifmm_state) != BLITZAR_STATUS_OK) {
            return false;
        }
    }

    blitzar_physics::ConservationMetrics direct_final{};
    blitzar_physics::ConservationMetrics kifmm_final{};

    if (blitzar_physics::ComputeConservationMetrics(
            direct_particles.State(), gravity, direct_final) != BLITZAR_STATUS_OK ||
        blitzar_physics::ComputeConservationMetrics(
            kifmm_particles.State(), gravity, kifmm_final) != BLITZAR_STATUS_OK) {
        return false;
    }

    const auto MomentumNorm = [](const blitzar_physics::ConservationMetrics& metrics) noexcept {
        return std::sqrt(metrics.momentum.x * metrics.momentum.x +
                         metrics.momentum.y * metrics.momentum.y +
                         metrics.momentum.z * metrics.momentum.z);
    };

    return direct_final.IsFinite() && kifmm_final.IsFinite() &&
           std::abs(direct_final.total_energy - direct_initial.total_energy) < 1.0e-8 &&
           std::abs(kifmm_final.total_energy - kifmm_initial.total_energy) < 1.0e-8 &&
           MomentumNorm(direct_final) < 1.0e-12 && MomentumNorm(kifmm_final) < 1.0e-12 &&
           SameParticleState(direct_particles.State(), kifmm_particles.State());
}

[[nodiscard]] bool RunEmptyAndDegenerateCases() noexcept
{
    constexpr std::size_t particle_count = 4;
    const auto settings = MakeSettings(particle_count, 0.0);
    auto resources = MakeResources(settings, particle_count);
    blitzar_kifmm::KifmmSolver kifmm({1.0, 0.1}, settings, particle_count, resources);
    blitzar_particles::ParticleBuffer empty_particles(0);
    blitzar_particles::ParticleAccelerationBuffer empty_forces(0);
    const blitzar_core::ExecutionSettings execution{};

    if (blitzar_tests::EvaluateLocal(
            kifmm, empty_particles.State(), empty_forces.View(), execution) != BLITZAR_STATUS_OK) {
        return false;
    }

    blitzar_particles::ParticleBuffer degenerate_particles(particle_count);
    blitzar_particles::ParticleAccelerationBuffer degenerate_forces(particle_count);

    for (std::size_t index = 0; index < particle_count; ++index) {
        if (degenerate_particles.SetPosition(index, {}) != BLITZAR_STATUS_OK ||

            degenerate_particles.SetVelocity(index, {}) != BLITZAR_STATUS_OK ||
            degenerate_particles.SetMass(index, 1.0) != BLITZAR_STATUS_OK) {
            return false;
        }
    }

    if (blitzar_tests::EvaluateLocal(kifmm, degenerate_particles.State(), degenerate_forces.View(),
            execution) != BLITZAR_STATUS_OK) {
        return false;
    }

    const blitzar_core::ForceView forces = degenerate_forces.View();

    for (std::size_t index = 0; index < particle_count; ++index) {
        if (!std::isfinite(forces.x[index]) || !std::isfinite(forces.y[index]) ||
            !std::isfinite(forces.z[index]) || forces.x[index] != 0.0 || forces.y[index] != 0.0 ||
            forces.z[index] != 0.0) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] bool RunCapacityCase() noexcept
{
    constexpr std::size_t capacity = 8;
    const auto settings = MakeSettings(capacity, 0.35);
    auto resources = MakeResources(settings, capacity);
    blitzar_kifmm::KifmmSolver kifmm({1.0, 0.1}, settings, capacity, resources);
    blitzar_particles::ParticleBuffer particles(capacity);
    blitzar_particles::ParticleAccelerationBuffer forces(capacity);
    const blitzar_core::ExecutionSettings execution{};

    FillParticles(particles);

    if (blitzar_tests::EvaluateLocal(kifmm, particles.State(), forces.View(), execution) !=
        BLITZAR_STATUS_OK) {
        return false;
    }

    const std::uint64_t generation = resources.Local().View().Generation();
    blitzar_particles::ParticleBuffer oversized_particles(capacity + 1);
    blitzar_particles::ParticleAccelerationBuffer oversized_forces(capacity + 1);

    for (std::size_t index = 0; index < capacity + 1; ++index) {
        oversized_forces.View().x[index] = 7.0;
        oversized_forces.View().y[index] = 8.0;
        oversized_forces.View().z[index] = 9.0;
    }

    const blitzar_status status = blitzar_tests::EvaluateLocal(
        kifmm, oversized_particles.State(), oversized_forces.View(), execution);

    for (std::size_t index = 0; index < capacity + 1; ++index) {
        if (oversized_forces.View().x[index] != 7.0 || oversized_forces.View().y[index] != 8.0 ||
            oversized_forces.View().z[index] != 9.0) {
            return false;
        }
    }

    return status == BLITZAR_STATUS_INVALID_ARGUMENT &&
           resources.Local().View().Generation() == generation;
}

[[nodiscard]] bool RunSingularityCase() noexcept
{
    constexpr std::size_t particle_count = 3;
    const auto settings = MakeSettings(particle_count, 0.0);
    auto resources = MakeResources(settings, particle_count);
    blitzar_kifmm::KifmmSolver kifmm({1.0, 0.0}, settings, particle_count, resources);
    blitzar_particles::ParticleBuffer particles(particle_count);
    blitzar_particles::ParticleAccelerationBuffer forces(particle_count);

    if (particles.SetPosition(0, {0.0, 0.0, 0.0}) != BLITZAR_STATUS_OK ||

        particles.SetPosition(1, {1.0, 0.0, 0.0}) != BLITZAR_STATUS_OK ||
        particles.SetPosition(2, {1.0, 0.0, 0.0}) != BLITZAR_STATUS_OK ||
        particles.SetMass(0, 1.0) != BLITZAR_STATUS_OK ||
        particles.SetMass(1, 1.0) != BLITZAR_STATUS_OK ||
        particles.SetMass(2, 1.0) != BLITZAR_STATUS_OK) {
        return false;
    }

    for (std::size_t index = 0; index < particle_count; ++index) {
        forces.View().x[index] = 4.0;
        forces.View().y[index] = 5.0;
        forces.View().z[index] = 6.0;
    }

    const blitzar_status status = blitzar_tests::EvaluateLocal(
        kifmm, particles.State(), forces.View(), blitzar_core::ExecutionSettings{});

    if (status != BLITZAR_STATUS_SINGULARITY) {
        return false;
    }

    for (std::size_t index = 0; index < particle_count; ++index) {
        if (forces.View().x[index] != 4.0 || forces.View().y[index] != 5.0 ||
            forces.View().z[index] != 6.0) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] bool RunFailedSecondEvaluationCase() noexcept
{
    constexpr std::size_t particle_count = SnapshotCount;
    const auto settings = MakeSettings(particle_count, 0.0);
    auto resources = MakeResources(settings, particle_count);
    blitzar_kifmm::KifmmSolver kifmm({1.0, 0.0}, settings, particle_count, resources);
    blitzar_particles::ParticleBuffer particles(particle_count);
    blitzar_particles::ParticleAccelerationBuffer forces(particle_count);

    if (particles.SetPosition(0, {0.0, 0.0, 0.0}) != BLITZAR_STATUS_OK ||

        particles.SetPosition(1, {1.0, 0.0, 0.0}) != BLITZAR_STATUS_OK ||
        particles.SetPosition(2, {0.0, 1.0, 0.0}) != BLITZAR_STATUS_OK ||
        particles.SetMass(0, 1.0) != BLITZAR_STATUS_OK ||
        particles.SetMass(1, 2.0) != BLITZAR_STATUS_OK ||
        particles.SetMass(2, 3.0) != BLITZAR_STATUS_OK ||
        blitzar_tests::EvaluateLocal(kifmm, particles.State(), forces.View(),
            blitzar_core::ExecutionSettings{}) != BLITZAR_STATUS_OK) {
        return false;
    }

    particles.MutableView().x[0] = std::numeric_limits<double>::quiet_NaN();

    const ParticleSnapshot particle_snapshot = CaptureState(particles.State());
    const ForceSnapshot force_snapshot = CaptureForces(forces.View());
    const std::uint64_t generation = resources.Local().View().Generation();

    const blitzar_status status = blitzar_tests::EvaluateLocal(
        kifmm, particles.State(), forces.View(), blitzar_core::ExecutionSettings{});

    return status == BLITZAR_STATUS_INVALID_ARGUMENT &&
           SameParticleState(particles.State(), particle_snapshot) &&
           SameForces(forces.View(), force_snapshot) &&
           resources.Local().View().Generation() == generation;
}

} // namespace

int main()
{
    BLITZAR_CHECK(RunOracleComparison());
    BLITZAR_CHECK(RunOrderingAndRepeatability());
    BLITZAR_CHECK(RunConservationCase());
    BLITZAR_CHECK(RunEmptyAndDegenerateCases());
    BLITZAR_CHECK(RunCapacityCase());
    BLITZAR_CHECK(RunSingularityCase());
    BLITZAR_CHECK(RunFailedSecondEvaluationCase());

    return 0;
}
