#ifndef BLITZAR_SOLVERS_BARNES_HUT_BARNES_HUT_SOLVER_HPP
#define BLITZAR_SOLVERS_BARNES_HUT_BARNES_HUT_SOLVER_HPP

#include "core/Execution.hpp"
#include "core/Solver.hpp"
#include "physics/GravityLaw.hpp"
#include "solvers/barnes_hut/ThreadStackPool.hpp"
#include "trees/Octree.hpp"

#include <blitzar/blitzar.h>
#include <cstddef>
#include <memory>
#include <span>
#include <vector>

namespace blitzar_barnes_hut {

struct BarnesHutSettings final {
    blitzar_core::Scalar opening_angle{0.5};
    std::size_t max_particles{};
    std::size_t max_cells{};
    std::size_t leaf_capacity{8};
    std::size_t max_depth{32};

    [[nodiscard]] bool IsValid() const noexcept;
};

struct BarnesHutSplitRequest final {
    blitzar_core::ParticleStateView local;
    blitzar_core::ParticleStateView remote;
    blitzar_core::ForceView forces;
    blitzar_core::ExecutionSettings settings;
};

class BarnesHutSolver final {
public:
    BarnesHutSolver(blitzar_physics::GravityParameters gravity, BarnesHutSettings settings,
        std::size_t local_particle_capacity = 0);

    [[nodiscard]] blitzar_core::SolverKind Kind() const noexcept;
    [[nodiscard]] blitzar_status Prepare(std::size_t particle_capacity) noexcept;
    [[nodiscard]] blitzar_status Compute(blitzar_core::ParticleStateView particles,
        blitzar_core::ForceView forces, const blitzar_core::ExecutionSettings& settings) noexcept;
    [[nodiscard]] blitzar_status Compute(blitzar_core::ParticleStateView particles,
        blitzar_core::ForceView forces, const blitzar_core::ExecutionSettings& settings,
        ThreadStackPool& stack_pool) noexcept;
    [[nodiscard]] blitzar_status ComputeSplit(const BarnesHutSplitRequest& request) noexcept;
    [[nodiscard]] blitzar_status ComputeSplit(
        const BarnesHutSplitRequest& request, ThreadStackPool& stack_pool) noexcept;

private:
    struct AccumulationRequest final {
        const blitzar_trees::Octree& tree;
        blitzar_core::ParticleStateView targets;
        blitzar_core::ParticleStateView sources;
        std::size_t target{};
        std::span<std::size_t> stack;
        blitzar_core::Vector3& acceleration;
        bool skip_self{false};
    };

    struct TreeComputeRequest final {
        blitzar_trees::Octree& tree;
        blitzar_core::ParticleStateView targets;
        blitzar_core::ParticleStateView sources;
        blitzar_core::ForceView forces;
        blitzar_core::ExecutionSettings settings;
        ThreadStackPool& stack_pool;
        bool accumulate{false};
        bool skip_self{false};
    };

    [[nodiscard]] static bool IsValidState(blitzar_core::ParticleStateView particles) noexcept;
    [[nodiscard]] static bool Contains(
        const blitzar_trees::Octree::Cell& cell, blitzar_core::Vector3 position) noexcept;
    [[nodiscard]] blitzar_status AccumulateSource(const AccumulationRequest& request,
        std::size_t source, blitzar_core::Vector3 target_position) const noexcept;
    [[nodiscard]] blitzar_status AccumulateLeaf(const AccumulationRequest& request,
        const blitzar_trees::Octree::Cell& cell,
        blitzar_core::Vector3 target_position) const noexcept;
    [[nodiscard]] blitzar_status AccumulateMultipole(const AccumulationRequest& request,
        const blitzar_trees::Octree::Cell& cell, blitzar_core::Vector3 target_position,
        bool& consumed) const noexcept;
    [[nodiscard]] static blitzar_status PushChildren(
        const blitzar_trees::Octree::Cell& cell, std::span<std::size_t> stack,
        std::size_t& stack_size) noexcept;
    [[nodiscard]] blitzar_status Accumulate(const AccumulationRequest& request) noexcept;
    [[nodiscard]] bool ValidateTreeRequest(const TreeComputeRequest& request) const noexcept;
    [[nodiscard]] blitzar_status PrepareTree(const TreeComputeRequest& request) noexcept;
    [[nodiscard]] blitzar_status ComputeTargets(const TreeComputeRequest& request) noexcept;
    [[nodiscard]] blitzar_status ComputeTree(const TreeComputeRequest& request) noexcept;
    [[nodiscard]] blitzar_status CommitStagedForces(blitzar_core::ForceView forces) noexcept;
    [[nodiscard]] blitzar_status EnsureRemoteTree() noexcept;
    [[nodiscard]] blitzar_status EnsureLocalCapacity(std::size_t particle_capacity) noexcept;

    BarnesHutSettings settings_;
    blitzar_physics::GravityLaw gravity_;
    std::size_t local_particle_capacity_;
    std::size_t local_cell_capacity_;
    std::unique_ptr<blitzar_trees::Octree> tree_;
    std::unique_ptr<blitzar_trees::Octree> remote_tree_;
    ThreadStackPool stack_pool_;
    std::vector<blitzar_core::Vector3> staging_;
};

} // namespace blitzar_barnes_hut

#endif
