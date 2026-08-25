#ifndef BLITZAR_SOLVERS_FMM_FMM_SOLVER_HPP
#define BLITZAR_SOLVERS_FMM_FMM_SOLVER_HPP

#include "core/Execution.hpp"
#include "core/Solver.hpp"
#include "physics/GravityLaw.hpp"
#include "solvers/barnes_hut/BarnesHutSolver.hpp"
#include "solvers/barnes_hut/ThreadStackPool.hpp"
#include "trees/Octree.hpp"

#include <blitzar/blitzar.h>
#include <cstddef>
#include <memory>
#include <span>
#include <vector>

namespace blitzar_fmm {

using FmmSettings = blitzar_barnes_hut::BarnesHutSettings;
using FmmSplitRequest = blitzar_barnes_hut::BarnesHutSplitRequest;

class FmmSolver final {
public:
    FmmSolver(blitzar_physics::GravityParameters gravity, FmmSettings settings,
        std::size_t local_particle_capacity = 0);

    [[nodiscard]] blitzar_core::SolverKind Kind() const noexcept;
    [[nodiscard]] blitzar_status Prepare(std::size_t particle_capacity) noexcept;
    [[nodiscard]] blitzar_status Compute(blitzar_core::ParticleStateView particles,
        blitzar_core::ForceView forces, const blitzar_core::ExecutionSettings& settings) noexcept;
    [[nodiscard]] blitzar_status Compute(blitzar_core::ParticleStateView particles,
        blitzar_core::ForceView forces, const blitzar_core::ExecutionSettings& settings,
        blitzar_barnes_hut::ThreadStackPool& stack_pool) noexcept;
    [[nodiscard]] blitzar_status ComputeSplit(const FmmSplitRequest& request) noexcept;
    [[nodiscard]] blitzar_status ComputeSplit(
        const FmmSplitRequest& request, blitzar_barnes_hut::ThreadStackPool& stack_pool) noexcept;
    [[nodiscard]] std::size_t BuildCount() const noexcept;
    [[nodiscard]] std::size_t RefitCount() const noexcept;

private:
    struct Multipole final {
        blitzar_core::Scalar mass{};
        blitzar_core::Scalar xx{};
        blitzar_core::Scalar xy{};
        blitzar_core::Scalar xz{};
        blitzar_core::Scalar yy{};
        blitzar_core::Scalar yz{};
        blitzar_core::Scalar zz{};
    };

    struct AccumulationRequest final {
        const blitzar_trees::Octree& tree;
        const std::vector<Multipole>& multipoles;
        blitzar_core::ParticleStateView targets;
        blitzar_core::ParticleStateView sources;
        std::size_t target{};
        std::span<std::size_t> stack;
        blitzar_core::Vector3& acceleration;
        bool skip_self{false};
    };

    struct TreeComputeRequest final {
        blitzar_trees::Octree& tree;
        std::vector<Multipole>& multipoles;
        blitzar_core::ParticleStateView targets;
        blitzar_core::ParticleStateView sources;
        blitzar_core::ForceView forces;
        blitzar_core::ExecutionSettings settings;
        blitzar_barnes_hut::ThreadStackPool& stack_pool;
        bool accumulate{false};
        bool skip_self{false};
    };

    [[nodiscard]] static bool IsValidState(blitzar_core::ParticleStateView particles) noexcept;
    [[nodiscard]] static bool Contains(
        const blitzar_trees::Octree::Cell& cell, blitzar_core::Vector3 position) noexcept;
    [[nodiscard]] static blitzar_status PairStatusToStatus(
        blitzar_physics::PairStatus status) noexcept;
    [[nodiscard]] static bool IsFiniteMultipole(const Multipole& multipole) noexcept;
    [[nodiscard]] blitzar_status BuildLeafMultipole(const blitzar_trees::Octree& tree,
        const blitzar_trees::Octree::Cell& cell, blitzar_core::ParticleStateView sources,
        Multipole& multipole) const noexcept;
    [[nodiscard]] blitzar_status MergeChildMultipoles(
        std::span<const blitzar_trees::Octree::Cell> cells,
        const blitzar_trees::Octree::Cell& cell, std::span<const Multipole> multipoles,
        Multipole& result) const noexcept;
    [[nodiscard]] static std::size_t LocalCellCapacity(
        std::size_t configured_cells, std::size_t particle_capacity) noexcept;
    [[nodiscard]] blitzar_status BuildMultipoles(const blitzar_trees::Octree& tree,
        blitzar_core::ParticleStateView sources, std::vector<Multipole>& multipoles) const noexcept;
    [[nodiscard]] blitzar_status EvaluateMultipole(const Multipole& multipole,
        blitzar_core::Vector3 displacement, blitzar_core::Scalar squared_distance,
        blitzar_core::Vector3& acceleration) const noexcept;
    [[nodiscard]] blitzar_status AccumulateLeaf(const AccumulationRequest& request,
        const blitzar_trees::Octree::Cell& cell, blitzar_core::Vector3 target_position) const noexcept;
    [[nodiscard]] static blitzar_status PushChildren(const AccumulationRequest& request,
        const blitzar_trees::Octree::Cell& cell, std::size_t& stack_size) noexcept;
    [[nodiscard]] blitzar_status ProcessCell(const AccumulationRequest& request,
        std::size_t cell_index, blitzar_core::Vector3 target_position,
        std::size_t& stack_size) const noexcept;
    [[nodiscard]] blitzar_status Accumulate(const AccumulationRequest& request) const noexcept;
    [[nodiscard]] bool IsKnownTree(const TreeComputeRequest& request) const noexcept;
    [[nodiscard]] bool HasValidTreeState(
        const TreeComputeRequest& request, std::size_t source_capacity) const noexcept;
    [[nodiscard]] bool HasValidTreeResources(const TreeComputeRequest& request) const noexcept;
    [[nodiscard]] bool ValidateTreeRequest(const TreeComputeRequest& request) const noexcept;
    [[nodiscard]] blitzar_status PrepareTree(const TreeComputeRequest& request) noexcept;
    [[nodiscard]] blitzar_status ComputeTargets(const TreeComputeRequest& request) noexcept;
    [[nodiscard]] blitzar_status ComputeTree(const TreeComputeRequest& request) noexcept;
    [[nodiscard]] blitzar_status CommitStagedForces(blitzar_core::ForceView forces) noexcept;
    [[nodiscard]] blitzar_status EnsureLocalCapacity(std::size_t particle_capacity) noexcept;

    FmmSettings settings_;
    blitzar_physics::GravityParameters parameters_;
    blitzar_physics::GravityLaw gravity_;
    std::size_t local_particle_capacity_;
    std::size_t local_cell_capacity_;
    std::unique_ptr<blitzar_trees::Octree> tree_;
    std::unique_ptr<blitzar_trees::Octree> remote_tree_;
    blitzar_barnes_hut::ThreadStackPool stack_pool_;
    std::vector<Multipole> multipoles_;
    std::vector<Multipole> remote_multipoles_;
    std::vector<blitzar_core::Vector3> staging_;
};

} // namespace blitzar_fmm

#endif
