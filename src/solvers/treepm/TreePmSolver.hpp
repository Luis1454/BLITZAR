#ifndef BLITZAR_SOLVERS_TREEPM_TREE_PM_SOLVER_HPP
#define BLITZAR_SOLVERS_TREEPM_TREE_PM_SOLVER_HPP

#include "solvers/SolverContract.hpp"
#include "solvers/SolverForceRequest.hpp"
#include "solvers/barnes_hut/BhSolver.hpp"
#include "solvers/pm/PmSolver.hpp"

#include <blitzar/blitzar.h>
#include <cstddef>
#include <functional>
#include <vector>

namespace blitzar_treepm {

struct TreePmSettings final {
    blitzar_core::Scalar pm_weight{0.5};
    blitzar_core::Scalar tree_weight{0.5};

    [[nodiscard]] bool IsValid() const noexcept;
};

class TreePmSolver final {
public:
    TreePmSolver(blitzar_pm::PmSolver pm, blitzar_barnes_hut::BhSolver tree,
        blitzar_solvers::SolverTreeResources& resources);

    TreePmSolver(const TreePmSolver&) = delete;
    TreePmSolver& operator=(const TreePmSolver&) = delete;
    TreePmSolver(TreePmSolver&& other) noexcept;
    TreePmSolver& operator=(TreePmSolver&& other) noexcept;

    void BindResources(blitzar_solvers::SolverTreeResources& resources) noexcept;

    [[nodiscard]] blitzar_solvers::SolverKind Kind() const noexcept;
    [[nodiscard]] blitzar_status Prepare(std::size_t staging_capacity) noexcept;
    [[nodiscard]] blitzar_status Evaluate(
        const blitzar_solvers::SolverForceRequest::TreePm& request) noexcept;
    [[nodiscard]] blitzar_grid::GridResource& GridResource() noexcept;
    [[nodiscard]] const blitzar_grid::GridResource& GridResource() const noexcept;
    [[nodiscard]] blitzar_solvers::SolverTreeResources& Resources() noexcept;
    [[nodiscard]] const blitzar_solvers::SolverTreeResources& Resources() const noexcept;

private:
    [[nodiscard]] static bool IsValidState(blitzar_core::ParticleStateView particles) noexcept;
    [[nodiscard]] bool ValidateRequest(
        const blitzar_solvers::SolverForceRequest::TreePm& request) const noexcept;
    [[nodiscard]] blitzar_status EvaluateComponents(
        const blitzar_solvers::SolverForceRequest::TreePm& request) noexcept;
    [[nodiscard]] blitzar_status Commit(
        const blitzar_solvers::SolverForceRequest::TreePm& request) noexcept;
    [[nodiscard]] blitzar_status EnsureCapacity(std::size_t staging_capacity) noexcept;

    TreePmSettings settings_{};
    blitzar_pm::PmSolver pm_;
    blitzar_barnes_hut::BhSolver tree_;
    std::reference_wrapper<blitzar_solvers::SolverTreeResources> resources_;
    std::size_t staging_capacity_{};
    std::vector<blitzar_core::Scalar> pm_x_{};
    std::vector<blitzar_core::Scalar> pm_y_{};
    std::vector<blitzar_core::Scalar> pm_z_{};
    std::vector<blitzar_core::Scalar> tree_x_{};
    std::vector<blitzar_core::Scalar> tree_y_{};
    std::vector<blitzar_core::Scalar> tree_z_{};
};

} // namespace blitzar_treepm

#endif
