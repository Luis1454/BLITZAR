#include "core/Execution.hpp"
#include "particles/ParticleBuffer.hpp"
#include "solvers/barnes_hut/BarnesHutSolver.hpp"
#include "solvers/direct/DirectSolver.hpp"

#include <cassert>
#include <cmath>

int main()
{
    blitzar_particles::ParticleBuffer direct_particles(4);
    blitzar_particles::ParticleBuffer tree_particles(4);
    const blitzar_core::Vector3 positions[] = {
        {-1.0, -1.0, -1.0},
        {1.0, -1.0, 1.0},
        {-1.0, 1.0, 1.0},
        {1.0, 1.0, -1.0}};
    for (std::size_t index = 0; index < 4; ++index) {
        direct_particles.SetPosition(index, positions[index]);
        tree_particles.SetPosition(index, positions[index]);
        direct_particles.SetMass(index, 1.0 + static_cast<double>(index));
        tree_particles.SetMass(index, 1.0 + static_cast<double>(index));
    }

    const blitzar_core::ExecutionSettings execution{};
    const blitzar_physics::GravityParameters gravity{1.0, 0.0};
    blitzar_direct::DirectSolver direct_solver(gravity);
    blitzar_barnes_hut::BarnesHutSettings settings{};
    settings.opening_angle = 0.0;
    settings.max_particles = 4;
    settings.max_cells = 128;
    settings.leaf_capacity = 1;
    settings.max_depth = 8;
    blitzar_barnes_hut::BarnesHutSolver tree_solver(gravity, settings);
    blitzar_particles::AccelerationBuffer direct_acceleration(4);
    blitzar_particles::AccelerationBuffer tree_acceleration(4);
    assert(direct_solver.Compute(
               direct_particles.State(), direct_acceleration.View(), execution) ==
           BLITZAR_STATUS_OK);
    assert(tree_solver.Compute(
               tree_particles.State(), tree_acceleration.View(), execution) ==
           BLITZAR_STATUS_OK);
    assert(tree_solver.Kind() == blitzar_core::SolverKind::BarnesHut);

    const blitzar_core::ForceView direct_force = direct_acceleration.View();
    const blitzar_core::ForceView tree_force = tree_acceleration.View();
    for (std::size_t index = 0; index < 4; ++index) {
        assert(std::abs(direct_force.x[index] - tree_force.x[index]) < 1.0e-12);
        assert(std::abs(direct_force.y[index] - tree_force.y[index]) < 1.0e-12);
        assert(std::abs(direct_force.z[index] - tree_force.z[index]) < 1.0e-12);
    }
    return 0;
}
