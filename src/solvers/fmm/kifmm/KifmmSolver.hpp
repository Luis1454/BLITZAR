#ifndef BLITZAR_SOLVERS_FMM_KIFMM_KIFMM_SOLVER_HPP
#define BLITZAR_SOLVERS_FMM_KIFMM_KIFMM_SOLVER_HPP

#include "core/CoreExecution.hpp"
#include "physics/gravity/GravityLaw.hpp"
#include "solvers/SolverContract.hpp"
#include "solvers/SolverForceRequest.hpp"
#include "solvers/SolverTreeResources.hpp"
#include "solvers/fmm/kifmm/KifmmWorkspace.hpp"

#include <blitzar/blitzar.h>
#include <cstddef>
#include <functional>
#include <span>

namespace blitzar_kifmm {

struct KifmmSettings final {
    blitzar_core::Scalar opening_angle{0.35};
    std::size_t max_particles{};
    std::size_t max_cells{};
    std::size_t leaf_capacity{8};
    std::size_t max_depth{32};

    [[nodiscard]] bool IsValid() const noexcept;
};

class KifmmSolver final {
public:
    KifmmSolver(blitzar_physics::GravityParameters gravity, KifmmSettings settings,
        std::size_t local_particle_capacity, blitzar_solvers::SolverTreeResources& resources);

    KifmmSolver(const KifmmSolver&) = delete;
    KifmmSolver& operator=(const KifmmSolver&) = delete;
    KifmmSolver(KifmmSolver&&) noexcept = default;
    KifmmSolver& operator=(KifmmSolver&&) noexcept = default;

    void BindResources(blitzar_solvers::SolverTreeResources& resources) noexcept;

    [[nodiscard]] blitzar_solvers::SolverKind Kind() const noexcept;
    [[nodiscard]] blitzar_status Prepare(std::size_t particle_capacity) noexcept;
    [[nodiscard]] blitzar_status Evaluate(
        const blitzar_solvers::SolverForceRequest::Tree& request) noexcept;
    [[nodiscard]] blitzar_solvers::SolverTreeResources& Resources() noexcept;
    [[nodiscard]] const blitzar_solvers::SolverTreeResources& Resources() const noexcept;
    [[nodiscard]] std::size_t BuildCount() const noexcept;
    [[nodiscard]] std::size_t RefitCount() const noexcept;

private:
    struct TreeComputeRequest final {
        const blitzar_trees::OctreeResource& resource;
        blitzar_trees::OctreeView tree;
        blitzar_core::ParticleStateView targets;
        blitzar_core::ParticleStateView sources;
        blitzar_core::ForceView forces;
        const blitzar_core::ExecutionSettings& settings;
        bool accumulate{false};
        bool skip_self{false};
    };

    struct InteractionList final {
        std::span<std::size_t> stack;
        std::span<std::size_t> near_cells;
        std::span<std::size_t> far_cells;
        std::size_t near_count{};
        std::size_t far_count{};
    };

    [[nodiscard]] static bool IsValidState(blitzar_core::ParticleStateView particles) noexcept;
    [[nodiscard]] static std::size_t LocalCellCapacity(
        std::size_t configured_cells, std::size_t particle_capacity) noexcept;
    [[nodiscard]] static blitzar_status PairStatusToStatus(
        blitzar_physics::PairStatus status) noexcept;
    [[nodiscard]] blitzar_status BuildOperator(std::size_t depth, blitzar_core::Scalar half_extent,
        blitzar_core::Scalar softening) noexcept;
    [[nodiscard]] blitzar_status BuildOperators(const blitzar_trees::OctreeView& tree) noexcept;
    [[nodiscard]] blitzar_status BuildEquivalentWeights(const TreeComputeRequest& request) noexcept;
    [[nodiscard]] blitzar_status BuildCellWeights(
        const TreeComputeRequest& request, std::size_t cell_index) noexcept;
    [[nodiscard]] bool ValidateTreeRequest(const TreeComputeRequest& request) const noexcept;
    [[nodiscard]] blitzar_status BuildInteractions(
        const TreeComputeRequest& request, std::size_t target, InteractionList& list) noexcept;
    [[nodiscard]] blitzar_status AccumulateNearCell(const TreeComputeRequest& request,
        std::size_t target, std::size_t cell_index,
        blitzar_core::Vector3& acceleration) const noexcept;
    [[nodiscard]] blitzar_status AccumulateFarCell(const TreeComputeRequest& request,
        const blitzar_core::Vector3& target_position, std::size_t cell_index,
        blitzar_core::Vector3& acceleration) const noexcept;
    [[nodiscard]] blitzar_status ComputeTarget(
        const TreeComputeRequest& request, std::size_t target) noexcept;
    [[nodiscard]] blitzar_status ComputeTargets(const TreeComputeRequest& request) noexcept;
    [[nodiscard]] blitzar_status ComputeTree(const TreeComputeRequest& request) noexcept;
    [[nodiscard]] blitzar_status CommitStagedForces(blitzar_core::ForceView forces) noexcept;
    [[nodiscard]] blitzar_status EnsureLocalCapacity(std::size_t particle_capacity) noexcept;

    KifmmSettings settings_;
    blitzar_physics::GravityParameters parameters_;
    blitzar_physics::GravityLaw gravity_;
    std::size_t local_particle_capacity_;
    std::size_t local_cell_capacity_;
    std::reference_wrapper<blitzar_solvers::SolverTreeResources> resources_;
    KifmmWorkspace workspace_;
};

} // namespace blitzar_kifmm

#endif
