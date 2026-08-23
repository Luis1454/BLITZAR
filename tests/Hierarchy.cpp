#include "core/Execution.hpp"
#include "particles/ParticleBuffer.hpp"
#include "solvers/barnes_hut/BarnesHutSolver.hpp"
#include "solvers/barnes_hut/ThreadWorkspace.hpp"
#include "solvers/direct/DirectSolver.hpp"
#include "trees/Octree.hpp"
#include "Check.hpp"

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
        BLITZAR_CHECK(direct_particles.SetPosition(index, positions[index]) ==
               BLITZAR_STATUS_OK);
        BLITZAR_CHECK(tree_particles.SetPosition(index, positions[index]) ==
               BLITZAR_STATUS_OK);
        BLITZAR_CHECK(direct_particles.SetMass(index, 1.0 + static_cast<double>(index)) ==
               BLITZAR_STATUS_OK);
        BLITZAR_CHECK(tree_particles.SetMass(index, 1.0 + static_cast<double>(index)) ==
               BLITZAR_STATUS_OK);
    }

    const blitzar_core::ExecutionSettings execution{};
    const blitzar_physics::GravityParameters gravity{1.0, 0.0};
    blitzar_direct::DirectSolver direct_solver(gravity);
    BLITZAR_CHECK(direct_solver.Prepare(8) == BLITZAR_STATUS_OK);
    blitzar_barnes_hut::BarnesHutSettings settings{};
    settings.opening_angle = 0.0;
    settings.max_particles = 4;
    settings.max_cells = 128;
    settings.leaf_capacity = 1;
    settings.max_depth = 8;
    blitzar_barnes_hut::BarnesHutSolver tree_solver(gravity, settings);
    blitzar_particles::AccelerationBuffer direct_acceleration(4);
    blitzar_particles::AccelerationBuffer tree_acceleration(4);
    BLITZAR_CHECK(direct_solver.Compute(
               direct_particles.State(), direct_acceleration.View(), execution) ==
           BLITZAR_STATUS_OK);
    BLITZAR_CHECK(tree_solver.Compute(
               tree_particles.State(), tree_acceleration.View(), execution) ==
           BLITZAR_STATUS_OK);
    blitzar_barnes_hut::ThreadWorkspace thread_workspace(
        settings.max_cells, settings.max_depth);
    BLITZAR_CHECK(thread_workspace.ThreadCount() > 0);
    BLITZAR_CHECK(thread_workspace.StackCapacity() > 0);
    BLITZAR_CHECK(thread_workspace.StackCapacity() <= settings.max_cells);
    BLITZAR_CHECK(tree_solver.Compute(
               tree_particles.State(),
               tree_acceleration.View(),
               execution,
               thread_workspace) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(tree_solver.Kind() == blitzar_core::SolverKind::BarnesHut);

    const blitzar_core::ForceView direct_force = direct_acceleration.View();
    const blitzar_core::ForceView tree_force = tree_acceleration.View();
    for (std::size_t index = 0; index < 4; ++index) {
        BLITZAR_CHECK(std::abs(direct_force.x[index] - tree_force.x[index]) < 1.0e-12);
        BLITZAR_CHECK(std::abs(direct_force.y[index] - tree_force.y[index]) < 1.0e-12);
        BLITZAR_CHECK(std::abs(direct_force.z[index] - tree_force.z[index]) < 1.0e-12);
    }

    blitzar_particles::ParticleBuffer zero_direct(2);
    blitzar_particles::ParticleBuffer zero_tree(2);
    for (std::size_t index = 0; index < 2; ++index) {
        BLITZAR_CHECK(zero_direct.SetPosition(index, {0.0, 0.0, 0.0}) ==
               BLITZAR_STATUS_OK);
        BLITZAR_CHECK(zero_tree.SetPosition(index, {0.0, 0.0, 0.0}) ==
               BLITZAR_STATUS_OK);
        BLITZAR_CHECK(zero_direct.SetMass(index, 0.0) == BLITZAR_STATUS_OK);
        BLITZAR_CHECK(zero_tree.SetMass(index, 0.0) == BLITZAR_STATUS_OK);
    }
    blitzar_particles::AccelerationBuffer zero_direct_force(2);
    blitzar_particles::AccelerationBuffer zero_tree_force(2);
    blitzar_barnes_hut::BarnesHutSolver zero_solver(gravity, settings);
    BLITZAR_CHECK(direct_solver.Compute(
               zero_direct.State(), zero_direct_force.View(), execution) ==
           BLITZAR_STATUS_OK);
    BLITZAR_CHECK(zero_solver.Compute(
               zero_tree.State(), zero_tree_force.View(), execution) ==
           BLITZAR_STATUS_OK);
    for (std::size_t index = 0; index < 2; ++index) {
        BLITZAR_CHECK(zero_direct_force.View().x[index] == 0.0);
        BLITZAR_CHECK(zero_tree_force.View().x[index] == 0.0);
    }

    blitzar_particles::ParticleBuffer sparse_particles(4);
    const blitzar_core::Vector3 sparse_positions[] = {
        {-1.0, 0.0, 0.0}, {-0.5, 0.0, 0.0}, {0.5, 0.0, 0.0}, {1.0, 0.0, 0.0}};
    for (std::size_t index = 0; index < 4; ++index) {
        BLITZAR_CHECK(sparse_particles.SetPosition(index, sparse_positions[index]) ==
               BLITZAR_STATUS_OK);
        BLITZAR_CHECK(sparse_particles.SetMass(index, 1.0) == BLITZAR_STATUS_OK);
    }
    blitzar_trees::Octree sparse_tree(4, 128, 1, 8);
    BLITZAR_CHECK(sparse_tree.Build(sparse_particles.State()) == BLITZAR_STATUS_OK);
    const auto sparse_root = sparse_tree.CellAt(0);
    BLITZAR_CHECK(!sparse_root.empty());
    BLITZAR_CHECK(!sparse_root.front().IsLeaf());
    BLITZAR_CHECK(sparse_tree.CellAt(128).empty());
    std::size_t invalid_particle = 0;
    BLITZAR_CHECK(!sparse_tree.ParticleIndex(4, invalid_particle));

    blitzar_particles::ParticleBuffer tight_particles(2);
    BLITZAR_CHECK(tight_particles.SetPosition(0, {-1.0, 0.0, 0.0}) ==
                  BLITZAR_STATUS_OK);
    BLITZAR_CHECK(tight_particles.SetPosition(1, {1.0, 0.0, 0.0}) ==
                  BLITZAR_STATUS_OK);
    BLITZAR_CHECK(tight_particles.SetMass(0, 1.0) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(tight_particles.SetMass(1, 1.0) == BLITZAR_STATUS_OK);
    blitzar_trees::Octree tight_tree(2, 3, 1, 8);
    BLITZAR_CHECK(tight_tree.Build(tight_particles.State()) ==
                  BLITZAR_STATUS_OK);
    BLITZAR_CHECK(tight_tree.CellCount() == 3);

    blitzar_particles::ParticleBuffer tree_singular(3);
    blitzar_particles::AccelerationBuffer tree_singular_force(3);
    BLITZAR_CHECK(tree_singular.SetPosition(0, {0.0, 0.0, 0.0}) ==
           BLITZAR_STATUS_OK);
    BLITZAR_CHECK(tree_singular.SetPosition(1, {1.0, 0.0, 0.0}) ==
           BLITZAR_STATUS_OK);
    BLITZAR_CHECK(tree_singular.SetPosition(2, {1.0, 0.0, 0.0}) ==
           BLITZAR_STATUS_OK);
    for (std::size_t index = 0; index < 3; ++index) {
        BLITZAR_CHECK(tree_singular.SetMass(index, 1.0) == BLITZAR_STATUS_OK);
    }
    const blitzar_core::ForceView tree_singular_view =
        tree_singular_force.View();
    for (std::size_t index = 0; index < 3; ++index) {
        tree_singular_view.x[index] = 4.0;
        tree_singular_view.y[index] = 5.0;
        tree_singular_view.z[index] = 6.0;
    }
    blitzar_barnes_hut::BarnesHutSolver singular_solver(gravity, settings);
    BLITZAR_CHECK(singular_solver.Compute(
               tree_singular.State(), tree_singular_view, execution) ==
           BLITZAR_STATUS_SINGULARITY);
    for (std::size_t index = 0; index < 3; ++index) {
        BLITZAR_CHECK(tree_singular_view.x[index] == 4.0);
        BLITZAR_CHECK(tree_singular_view.y[index] == 5.0);
        BLITZAR_CHECK(tree_singular_view.z[index] == 6.0);
    }

    blitzar_particles::ParticleBuffer clustered_direct(8);
    blitzar_particles::ParticleBuffer clustered_tree(8);
    for (std::size_t index = 0; index < 4; ++index) {
        const double offset = 0.1 * static_cast<double>(index);
        BLITZAR_CHECK(clustered_direct.SetPosition(
                   index, {-5.0 + offset, -5.0, -5.0}) == BLITZAR_STATUS_OK);
        BLITZAR_CHECK(clustered_tree.SetPosition(
                   index, {-5.0 + offset, -5.0, -5.0}) == BLITZAR_STATUS_OK);
        BLITZAR_CHECK(clustered_direct.SetPosition(
                   index + 4, {5.0 + offset, 5.0, 5.0}) == BLITZAR_STATUS_OK);
        BLITZAR_CHECK(clustered_tree.SetPosition(
                   index + 4, {5.0 + offset, 5.0, 5.0}) == BLITZAR_STATUS_OK);
    }
    blitzar_barnes_hut::BarnesHutSettings clustered_settings{};
    clustered_settings.opening_angle = 0.5;
    clustered_settings.max_particles = 8;
    clustered_settings.max_cells = 256;
    clustered_settings.leaf_capacity = 1;
    clustered_settings.max_depth = 8;
    blitzar_barnes_hut::BarnesHutSolver clustered_solver(
        gravity, clustered_settings);
    blitzar_particles::AccelerationBuffer clustered_direct_force(8);
    blitzar_particles::AccelerationBuffer clustered_tree_force(8);
    BLITZAR_CHECK(direct_solver.Compute(
               clustered_direct.State(), clustered_direct_force.View(), execution) ==
           BLITZAR_STATUS_OK);
    BLITZAR_CHECK(clustered_solver.Compute(
               clustered_tree.State(), clustered_tree_force.View(), execution) ==
           BLITZAR_STATUS_OK);
    const blitzar_core::ForceView exact_cluster_force =
        clustered_direct_force.View();
    const blitzar_core::ForceView approximate_cluster_force =
        clustered_tree_force.View();
    for (std::size_t index = 0; index < 8; ++index) {
        BLITZAR_CHECK(std::abs(
                   exact_cluster_force.x[index] - approximate_cluster_force.x[index]) <
               0.05);
        BLITZAR_CHECK(std::abs(
                   exact_cluster_force.y[index] - approximate_cluster_force.y[index]) <
               0.05);
        BLITZAR_CHECK(std::abs(
                   exact_cluster_force.z[index] - approximate_cluster_force.z[index]) <
               0.05);
    }

    blitzar_trees::Octree tree(8, 256, 1, 8);
    BLITZAR_CHECK(tree.Build(clustered_tree.State()) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(tree.BuildCount() == 1);
    BLITZAR_CHECK(tree.RefitCount() == 0);
    BLITZAR_CHECK(clustered_tree.SetPosition(0, {-4.95, -5.0, -5.0}) ==
           BLITZAR_STATUS_OK);
    BLITZAR_CHECK(tree.Refit(clustered_tree.State()));
    BLITZAR_CHECK(tree.BuildCount() == 1);
    BLITZAR_CHECK(tree.RefitCount() == 1);
    return 0;
}
