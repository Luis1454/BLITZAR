#include "Check.hpp"
#include "gpu/HipContext.hpp"
#include "particles/ParticleBuffer.hpp"
#include "sdk/Simulation.hpp"
#include "solvers/direct/DirectSolver.hpp"

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

[[nodiscard]] bool Configure(blitzar_sdk::Simulation& simulation, const TestState& state) noexcept
{
    return simulation.SetGravity(1.0, 0.1) == BLITZAR_STATUS_OK &&
           simulation.SetTimestep(0.01) == BLITZAR_STATUS_OK &&
           simulation.SetParticles(state.x, state.y, state.z, state.velocity_x, state.velocity_y,
               state.velocity_z, state.mass) == BLITZAR_STATUS_OK;
}

[[nodiscard]] bool RunDispatcherErrorCase() noexcept
{
    const TestState state{};
    blitzar_gpu::HipContext probe;
    blitzar_sdk::Simulation simulation(TestParticleCount);

    if (!Configure(simulation, state) || simulation.Step() != BLITZAR_STATUS_OK) {
        return false;
    }

    const blitzar_backend_kind expected_backend =
        probe.IsAvailable() ? BLITZAR_BACKEND_HIP : BLITZAR_BACKEND_CPU;

    if (simulation.LastBackend() != expected_backend) {
        return false;
    }

    const std::array<std::pair<blitzar_gpu::HipFault, blitzar_status>, 4> faults{
        {{blitzar_gpu::HipFault::AllocationFailure, BLITZAR_STATUS_ALLOCATION_FAILURE},
            {blitzar_gpu::HipFault::LaunchFailure, BLITZAR_STATUS_INTERNAL_ERROR},
            {blitzar_gpu::HipFault::SynchronizationFailure, BLITZAR_STATUS_INTERNAL_ERROR},
            {blitzar_gpu::HipFault::NonFiniteResult, BLITZAR_STATUS_INVALID_ARGUMENT}}};

    for (const auto& [fault, expected_status] : faults) {
        simulation.SetHipFaultForTesting(fault);

        if (simulation.Step() != expected_status ||
            simulation.LastBackend() != BLITZAR_BACKEND_HIP) {
            return false;
        }

        simulation.SetHipFaultForTesting(blitzar_gpu::HipFault::None);
    }

    blitzar_sdk::Simulation unsupported(TestParticleCount);

    if (!Configure(unsupported, state) ||
        unsupported.SetSolver(BLITZAR_SOLVER_BARNES_HUT) != BLITZAR_STATUS_OK ||
        unsupported.SetBarnesHut(0.5, TestParticleCount, 128, 1, 37) != BLITZAR_STATUS_OK ||
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
    blitzar_particles::AccelerationBuffer cpu_forces(4);
    blitzar_particles::AccelerationBuffer gpu_forces(4);
    blitzar_direct::DirectSolver cpu_solver(gravity);

    BLITZAR_CHECK(cpu_solver.Prepare(4) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(
        cpu_solver.Compute(particles.State(), cpu_forces.View(), execution) == BLITZAR_STATUS_OK);

    blitzar_gpu::HipContext context;

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

    for (std::size_t index = 0; index < particles.Count(); ++index) {
        BLITZAR_CHECK(std::abs(cpu_view.x[index] - gpu_view.x[index]) < 1.0e-5);
        BLITZAR_CHECK(std::abs(cpu_view.y[index] - gpu_view.y[index]) < 1.0e-5);
        BLITZAR_CHECK(std::abs(cpu_view.z[index] - gpu_view.z[index]) < 1.0e-5);
    }

    blitzar_barnes_hut::BarnesHutSettings settings{};

    settings.opening_angle = 0.0;
    settings.max_particles = 4;
    settings.max_cells = 128;
    settings.leaf_capacity = 1;
    settings.max_depth = 8;

    BLITZAR_CHECK(context.ComputeBarnesHut(particles.State(), gpu_forces.View(), execution, gravity,
                      settings) == BLITZAR_STATUS_OK);

    const blitzar_core::ForceView tree_view = gpu_forces.View();

    for (std::size_t index = 0; index < particles.Count(); ++index) {
        BLITZAR_CHECK(std::abs(cpu_view.x[index] - tree_view.x[index]) < 1.0e-5);
        BLITZAR_CHECK(std::abs(cpu_view.y[index] - tree_view.y[index]) < 1.0e-5);
        BLITZAR_CHECK(std::abs(cpu_view.z[index] - tree_view.z[index]) < 1.0e-5);
    }

    BLITZAR_CHECK(RunDispatcherErrorCase());

    return 0;
}
