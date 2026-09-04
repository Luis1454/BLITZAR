#ifndef BLITZAR_SOLVERS_SOLVER_CPU_FORCE_PROVIDER_HPP
#define BLITZAR_SOLVERS_SOLVER_CPU_FORCE_PROVIDER_HPP

#include "solvers/SolverForceEvaluation.hpp"
#include "solvers/SolverForceRequest.hpp"
#include "solvers/barnes_hut/BhSolver.hpp"
#include "solvers/direct/DirectSolver.hpp"
#include "solvers/fmm/FmmSolver.hpp"
#include "solvers/fmm/kifmm/KifmmSolver.hpp"
#include "solvers/pm/PmSolver.hpp"
#include "solvers/treepm/TreePmSolver.hpp"

namespace blitzar_solvers {

template <typename Solver> struct SolverCpuForceTraits;

template <> struct SolverCpuForceTraits<blitzar_direct::DirectSolver> final {
    [[nodiscard]] static blitzar_status Evaluate(
        blitzar_direct::DirectSolver& solver, const SolverForceEvaluation& request) noexcept
    {
        const blitzar_status prepare_status = solver.Prepare(request.targets.count);

        if (prepare_status != BLITZAR_STATUS_OK) {
            return prepare_status;
        }

        const bool local = request.source_kind == SolverForceSourceKind::Local;
        const SolverForceRequest::Direct typed_request{request.targets, request.sources,
            request.forces, request.settings, {0, request.sources.SourceCount(), !local}, local};

        return solver.Evaluate(typed_request);
    }
};

template <> struct SolverCpuForceTraits<blitzar_barnes_hut::BhSolver> final {
    [[nodiscard]] static blitzar_status Evaluate(
        blitzar_barnes_hut::BhSolver& solver, const SolverForceEvaluation& request) noexcept
    {
        const blitzar_status prepare_status = solver.Prepare(request.targets.count);

        if (prepare_status != BLITZAR_STATUS_OK) {
            return prepare_status;
        }

        const bool local = request.source_kind == SolverForceSourceKind::Local;

        blitzar_trees::OctreeResource& resource =
            local ? solver.Resources().Local() : solver.Resources().Remote();

        const blitzar_status resource_status = resource.Prepare(request.sources);

        if (resource_status != BLITZAR_STATUS_OK) {
            return resource_status;
        }

        const SolverForceRequest::Tree typed_request{request.targets, request.sources,
            request.forces, request.settings, resource, resource.View(), request.source_kind,
            !local, local};

        return solver.Evaluate(typed_request);
    }
};

template <> struct SolverCpuForceTraits<blitzar_fmm::FmmSolver> final {
    [[nodiscard]] static blitzar_status Evaluate(
        blitzar_fmm::FmmSolver& solver, const SolverForceEvaluation& request) noexcept
    {
        const blitzar_status prepare_status = solver.Prepare(request.targets.count);

        if (prepare_status != BLITZAR_STATUS_OK) {
            return prepare_status;
        }

        const bool local = request.source_kind == SolverForceSourceKind::Local;

        blitzar_trees::OctreeResource& resource =
            local ? solver.Resources().Local() : solver.Resources().Remote();

        const blitzar_status resource_status = resource.Prepare(request.sources);

        if (resource_status != BLITZAR_STATUS_OK) {
            return resource_status;
        }

        const SolverForceRequest::Tree typed_request{request.targets, request.sources,
            request.forces, request.settings, resource, resource.View(), request.source_kind,
            !local, local};

        return solver.Evaluate(typed_request);
    }
};

template <> struct SolverCpuForceTraits<blitzar_kifmm::KifmmSolver> final {
    [[nodiscard]] static blitzar_status Evaluate(
        blitzar_kifmm::KifmmSolver& solver, const SolverForceEvaluation& request) noexcept
    {
        const blitzar_status prepare_status = solver.Prepare(request.targets.count);

        if (prepare_status != BLITZAR_STATUS_OK) {
            return prepare_status;
        }

        const bool local = request.source_kind == SolverForceSourceKind::Local;

        blitzar_trees::OctreeResource& resource =
            local ? solver.Resources().Local() : solver.Resources().Remote();

        const blitzar_status resource_status = resource.Prepare(request.sources);

        if (resource_status != BLITZAR_STATUS_OK) {
            return resource_status;
        }

        const SolverForceRequest::Tree typed_request{request.targets, request.sources,
            request.forces, request.settings, resource, resource.View(), request.source_kind,
            !local, local};

        return solver.Evaluate(typed_request);
    }
};

template <> struct SolverCpuForceTraits<blitzar_pm::PmSolver> final {
    [[nodiscard]] static blitzar_status Evaluate(
        blitzar_pm::PmSolver& solver, const SolverForceEvaluation& request) noexcept
    {
        if (request.source_kind != SolverForceSourceKind::Local) {
            return BLITZAR_STATUS_UNSUPPORTED;
        }

        const blitzar_status prepare_status = solver.Prepare(request.targets.count);

        if (prepare_status != BLITZAR_STATUS_OK) {
            return prepare_status;
        }

        blitzar_grid::GridResource& resource = solver.Resource();

        const blitzar_status resource_status = resource.Prepare(request.sources);

        if (resource_status != BLITZAR_STATUS_OK) {
            return resource_status;
        }

        const SolverForceRequest::Grid typed_request{request.targets, request.sources,
            request.forces, request.settings, resource, resource.View(), request.source_kind, false,
            true};

        return solver.Evaluate(typed_request);
    }
};

template <> struct SolverCpuForceTraits<blitzar_treepm::TreePmSolver> final {
    [[nodiscard]] static blitzar_status Evaluate(
        blitzar_treepm::TreePmSolver& solver, const SolverForceEvaluation& request) noexcept
    {
        if (request.source_kind != SolverForceSourceKind::Local) {
            return BLITZAR_STATUS_UNSUPPORTED;
        }

        const blitzar_status prepare_status = solver.Prepare(request.targets.count);

        if (prepare_status != BLITZAR_STATUS_OK) {
            return prepare_status;
        }

        blitzar_grid::GridResource& grid = solver.GridResource();

        const blitzar_status grid_status = grid.Prepare(request.sources);

        if (grid_status != BLITZAR_STATUS_OK) {
            return grid_status;
        }

        blitzar_trees::OctreeResource& tree = solver.Resources().Local();

        const blitzar_status tree_status = tree.Prepare(request.sources);

        if (tree_status != BLITZAR_STATUS_OK) {
            return tree_status;
        }

        const SolverForceRequest::TreePm typed_request{request.targets, request.sources,
            request.forces, request.settings, grid, grid.View(), tree, tree.View(),
            request.source_kind, false, true};

        return solver.Evaluate(typed_request);
    }
};

template <typename Solver> class SolverCpuForceProvider final {
public:
    explicit SolverCpuForceProvider(Solver& solver) noexcept : solver_(solver) {}

    [[nodiscard]] blitzar_status Evaluate(const SolverForceEvaluation& request) noexcept
    {
        return SolverCpuForceTraits<Solver>::Evaluate(solver_, request);
    }

private:
    Solver& solver_;
};

} // namespace blitzar_solvers

#endif
