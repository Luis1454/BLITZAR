#include "gpu/HipContext.hpp"
#include "particles/ParticleBuffer.hpp"
#include "solvers/direct/DirectSolver.hpp"

#include "Check.hpp"

#include <cmath>

int main()
{
    blitzar_particles::ParticleBuffer particles(4);
    const blitzar_core::Vector3 positions[] = {
        {-1.0, -1.0, -1.0},
        {1.0, -1.0, 1.0},
        {-1.0, 1.0, 1.0},
        {1.0, 1.0, -1.0}};
    for (std::size_t index = 0; index < particles.Count(); ++index) {
        BLITZAR_CHECK(
            particles.SetPosition(index, positions[index]) ==
            BLITZAR_STATUS_OK);
        BLITZAR_CHECK(
            particles.SetMass(index, 1.0 + static_cast<double>(index)) ==
            BLITZAR_STATUS_OK);
    }

    const blitzar_physics::GravityParameters gravity{1.0, 0.1};
    const blitzar_core::ExecutionSettings execution{};
    blitzar_particles::AccelerationBuffer cpu_forces(4);
    blitzar_particles::AccelerationBuffer gpu_forces(4);
    blitzar_direct::DirectSolver cpu_solver(gravity);
    BLITZAR_CHECK(cpu_solver.Compute(
                      particles.State(), cpu_forces.View(), execution) ==
                  BLITZAR_STATUS_OK);

    blitzar_gpu::HipContext context;
    if (!context.IsAvailable()) {
        BLITZAR_CHECK(context.ComputeDirect(
                          particles.State(), gpu_forces.View(), gravity) ==
                      BLITZAR_STATUS_UNSUPPORTED);
        return 0;
    }

    BLITZAR_CHECK(context.IsCompiled());
    BLITZAR_CHECK(context.ComputeDirect(
                      particles.State(), gpu_forces.View(), gravity) ==
                  BLITZAR_STATUS_OK);
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
    BLITZAR_CHECK(context.ComputeBarnesHut(
                      particles.State(),
                      gpu_forces.View(),
                      execution,
                      gravity,
                      settings) == BLITZAR_STATUS_OK);
    const blitzar_core::ForceView tree_view = gpu_forces.View();
    for (std::size_t index = 0; index < particles.Count(); ++index) {
        BLITZAR_CHECK(std::abs(cpu_view.x[index] - tree_view.x[index]) < 1.0e-5);
        BLITZAR_CHECK(std::abs(cpu_view.y[index] - tree_view.y[index]) < 1.0e-5);
        BLITZAR_CHECK(std::abs(cpu_view.z[index] - tree_view.z[index]) < 1.0e-5);
    }
    return 0;
}
