#include "fixtures/FixtureCheck.hpp"
#include "fixtures/FixtureViews.hpp"
#include "gpu/runtime/GpuContext.hpp"
#include "particles/buffer/ParticleAccelerationBuffer.hpp"
#include "particles/buffer/ParticleBuffer.hpp"
#include "simulation/Sim.hpp"
#include "solvers/direct/DirectSolver.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <utility>

namespace {

constexpr std::size_t TestParticleCount = 4;

struct TestState final {
    std::array<double, TestParticleCount> x{-1.0, 1.0, -1.0, 1.0};
    std::array<double, TestParticleCount> y{-1.0, -1.0, 1.0, 1.0};
    std::array<double, TestParticleCount> z{-1.0, 1.0, 1.0, -1.0};
    std::array<double, TestParticleCount> velocity_x{};
    std::array<double, TestParticleCount> velocity_y{};
    std::array<double, TestParticleCount> velocity_z{};
    std::array<double, TestParticleCount> mass{1.0, 2.0, 3.0, 4.0};
};

[[nodiscard]] bool Configure(blitzar_sim::Sim& simulation, const TestState& state) noexcept
{
    return simulation.SetGravity(1.0, 0.1) == BLITZAR_STATUS_OK &&
           simulation.SetTimestep(0.01) == BLITZAR_STATUS_OK &&
           simulation.SetParticles(blitzar_tests::MakeStateView(state)) == BLITZAR_STATUS_OK;
}

[[nodiscard]] double MaxForceError(
    blitzar_core::ForceView reference, blitzar_core::ForceView candidate) noexcept
{
    double maximum = 0.0;

    for (std::size_t index = 0; index < reference.count; ++index) {
        maximum = std::max(maximum, std::abs(reference.x[index] - candidate.x[index]));
        maximum = std::max(maximum, std::abs(reference.y[index] - candidate.y[index]));
        maximum = std::max(maximum, std::abs(reference.z[index] - candidate.z[index]));
    }

    return maximum;
}

[[nodiscard]] bool RunDispatcherErrorCase() noexcept
{
    const TestState state{};
    blitzar_hip::GpuContext probe;
    blitzar_sim::Sim simulation(TestParticleCount);

    if (!Configure(simulation, state) || simulation.Step() != BLITZAR_STATUS_OK) {
        return false;
    }

    const blitzar_backend_kind expected_backend =
        probe.IsAvailable() ? BLITZAR_BACKEND_HIP : BLITZAR_BACKEND_CPU;

    if (simulation.LastBackend() != expected_backend) {
        return false;
    }

    const std::array<std::pair<blitzar_hip::Fault, blitzar_status>, 4> faults{
        {{blitzar_hip::Fault::AllocationFailure, BLITZAR_STATUS_ALLOCATION_FAILURE},
            {blitzar_hip::Fault::LaunchFailure, BLITZAR_STATUS_INTERNAL_ERROR},
            {blitzar_hip::Fault::SynchronizationFailure, BLITZAR_STATUS_INTERNAL_ERROR},
            {blitzar_hip::Fault::NonFiniteResult, BLITZAR_STATUS_INVALID_ARGUMENT}}};

    for (const auto& [fault, expected_status] : faults) {
        simulation.SetFaultForTesting(fault);

        if (simulation.Step() != expected_status ||
            simulation.LastBackend() != BLITZAR_BACKEND_HIP) {
            return false;
        }

        simulation.SetFaultForTesting(blitzar_hip::Fault::None);
    }

    blitzar_sim::Sim unsupported(TestParticleCount);

    if (!Configure(unsupported, state) ||
        unsupported.SetSolver(BLITZAR_SOLVER_BARNES_HUT) != BLITZAR_STATUS_OK ||
        unsupported.SetBarnesHut({0.5, TestParticleCount, 128, 1, 37}) != BLITZAR_STATUS_OK ||

        unsupported.Step() != BLITZAR_STATUS_OK ||
        unsupported.LastBackend() != BLITZAR_BACKEND_CPU) {
        return false;
    }

    return true;
}

} // namespace

int main()
{
    blitzar_particles::ParticleBuffer particles(4);
    const blitzar_core::Vector3 positions[] = {
        {-1.0, -1.0, -1.0}, {1.0, -1.0, 1.0}, {-1.0, 1.0, 1.0}, {1.0, 1.0, -1.0}};

    for (std::size_t index = 0; index < particles.Count(); ++index) {
        BLITZAR_CHECK(particles.SetPosition(index, positions[index]) == BLITZAR_STATUS_OK);
        BLITZAR_CHECK(
            particles.SetMass(index, 1.0 + static_cast<double>(index)) == BLITZAR_STATUS_OK);
    }

    const blitzar_physics::GravityParameters gravity{1.0, 0.1};
    const blitzar_core::ExecutionSettings execution{};
    blitzar_particles::ParticleAccelerationBuffer cpu_forces(4);
    blitzar_particles::ParticleAccelerationBuffer gpu_forces(4);
    blitzar_direct::DirectSolver cpu_solver(gravity);

    BLITZAR_CHECK(cpu_solver.Prepare(4) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(
        cpu_solver.Compute(particles.State(), cpu_forces.View(), execution) == BLITZAR_STATUS_OK);

    blitzar_hip::GpuContext context;

    BLITZAR_CHECK(RunDispatcherErrorCase());

    if (!context.IsAvailable()) {
        std::fprintf(stdout, "BLITZAR GPU qualification skipped: no compatible device is "
                             "visible; CPU fallback is being tested\n");

        BLITZAR_CHECK(context.ComputeDirect(particles.State(), gpu_forces.View(), gravity) ==
                      BLITZAR_STATUS_UNSUPPORTED);

        return 0;
    }

    BLITZAR_CHECK(context.IsCompiled());
    BLITZAR_CHECK(
        context.ComputeDirect(particles.State(), gpu_forces.View(), gravity) == BLITZAR_STATUS_OK);

    const blitzar_core::ForceView cpu_view = cpu_forces.View();
    const blitzar_core::ForceView gpu_view = gpu_forces.View();
    const double direct_max_error = MaxForceError(cpu_view, gpu_view);

    BLITZAR_CHECK(direct_max_error < 1.0e-5);

    blitzar_barnes_hut::BarnesHutSettings settings{};

    settings.opening_angle = 0.0;
    settings.max_particles = 4;
    settings.max_cells = 128;
    settings.leaf_capacity = 1;
    settings.max_depth = 8;

    BLITZAR_CHECK(context.ComputeBarnesHut({particles.State(), gpu_forces.View(), execution,
                      gravity, settings}) == BLITZAR_STATUS_OK);

    const blitzar_core::ForceView tree_view = gpu_forces.View();
    const double barnes_hut_max_error = MaxForceError(cpu_view, tree_view);

    BLITZAR_CHECK(barnes_hut_max_error < 1.0e-5);

    std::fprintf(stdout,
        "BLITZAR GPU qualification passed: compiled=%d device=1 direct_max_error=%.17g "
        "barnes_hut_max_error=%.17g fallback=1\n",
        context.IsCompiled() ? 1 : 0, direct_max_error, barnes_hut_max_error);

    return 0;
}
