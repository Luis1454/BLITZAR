#ifndef BLITZAR_SOLVERS_BARNES_HUT_BARNES_HUT_SOLVER_HPP
#define BLITZAR_SOLVERS_BARNES_HUT_BARNES_HUT_SOLVER_HPP

#include "core/Solver.hpp"
#include "core/Execution.hpp"
#include "physics/GravityLaw.hpp"
#include "trees/Octree.hpp"

#include <blitzar/blitzar.h>

#include <cstddef>
#include <span>

namespace blitzar_barnes_hut {

struct BarnesHutSettings final {
    blitzar_core::Scalar opening_angle{0.5};
    std::size_t max_particles{};
    std::size_t max_cells{};
    std::size_t leaf_capacity{8};
    std::size_t max_depth{32};

    [[nodiscard]] bool IsValid() const noexcept;
};

class BarnesHutSolver final {
public:
    BarnesHutSolver(
        blitzar_physics::GravityParameters gravity,
        BarnesHutSettings settings);

    [[nodiscard]] blitzar_core::SolverKind Kind() const noexcept;
    [[nodiscard]] blitzar_status Compute(
        blitzar_core::ParticleStateView particles,
        blitzar_core::ForceView forces,
        const blitzar_core::ExecutionSettings& settings) noexcept;
    [[nodiscard]] blitzar_status Compute(
        blitzar_core::ParticleStateView particles,
        blitzar_core::ForceView forces,
        const blitzar_core::ExecutionSettings& settings,
        std::span<std::size_t> traversal_stack) noexcept;

private:
    [[nodiscard]] static bool IsValidState(
        blitzar_core::ParticleStateView particles) noexcept;
    [[nodiscard]] static bool Contains(
        const blitzar_trees::Octree::Cell& cell,
        blitzar_core::Vector3 position) noexcept;
    [[nodiscard]] blitzar_status Accumulate(
        std::size_t target,
        blitzar_core::ParticleStateView particles,
        std::span<std::size_t> stack,
        blitzar_core::Vector3& acceleration) noexcept;

    BarnesHutSettings settings_;
    blitzar_physics::GravityLaw gravity_;
    blitzar_trees::Octree tree_;
};

}  // namespace blitzar_barnes_hut

#endif
