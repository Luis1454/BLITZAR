#ifndef BLITZAR_SIMULATION_SOLVER_SIM_SOLVER_VARIANT_HPP
#define BLITZAR_SIMULATION_SOLVER_SIM_SOLVER_VARIANT_HPP

#include "solvers/SolverTreeResources.hpp"
#include "solvers/barnes_hut/BhSolver.hpp"
#include "solvers/direct/DirectSolver.hpp"
#include "solvers/fmm/FmmSolver.hpp"
#include "solvers/pm/PmSolver.hpp"
#include "solvers/treepm/TreePmSolver.hpp"

#include <cstddef>
#include <utility>
#include <variant>

namespace blitzar_sim {

struct DirectSolverBundle final {
    DirectSolverBundle(blitzar_physics::GravityParameters gravity, std::size_t capacity)
        : solver(gravity, capacity)
    {
    }

    DirectSolverBundle(const DirectSolverBundle&) = delete;
    DirectSolverBundle& operator=(const DirectSolverBundle&) = delete;
    DirectSolverBundle(DirectSolverBundle&&) noexcept = default;
    DirectSolverBundle& operator=(DirectSolverBundle&&) noexcept = default;

    blitzar_direct::DirectSolver solver;
};

struct BarnesHutSolverBundle final {
    BarnesHutSolverBundle(blitzar_physics::GravityParameters gravity,
        blitzar_barnes_hut::BarnesHutSettings settings, std::size_t capacity)
        : resources({capacity, settings.max_cells, settings.leaf_capacity, settings.max_depth},
              {settings.max_particles, settings.max_cells, settings.leaf_capacity,
                  settings.max_depth}),
          solver(gravity, settings, capacity, resources)
    {
    }

    BarnesHutSolverBundle(const BarnesHutSolverBundle&) = delete;
    BarnesHutSolverBundle& operator=(const BarnesHutSolverBundle&) = delete;
    BarnesHutSolverBundle(BarnesHutSolverBundle&& other) noexcept
        : resources(std::move(other.resources)), solver(std::move(other.solver))
    {
        solver.BindResources(resources);
    }
    BarnesHutSolverBundle& operator=(BarnesHutSolverBundle&& other) noexcept
    {
        resources = std::move(other.resources);
        solver = std::move(other.solver);
        solver.BindResources(resources);

        return *this;
    }

    blitzar_solvers::SolverTreeResources resources;
    blitzar_barnes_hut::BhSolver solver;
};

struct FmmSolverBundle final {
    FmmSolverBundle(blitzar_physics::GravityParameters gravity, blitzar_fmm::FmmSettings settings,
        std::size_t capacity)
        : resources({capacity, settings.max_cells, settings.leaf_capacity, settings.max_depth},
              {settings.max_particles, settings.max_cells, settings.leaf_capacity,
                  settings.max_depth}),
          solver(gravity, settings, capacity, resources)
    {
    }

    FmmSolverBundle(const FmmSolverBundle&) = delete;
    FmmSolverBundle& operator=(const FmmSolverBundle&) = delete;
    FmmSolverBundle(FmmSolverBundle&& other) noexcept
        : resources(std::move(other.resources)), solver(std::move(other.solver))
    {
        solver.BindResources(resources);
    }
    FmmSolverBundle& operator=(FmmSolverBundle&& other) noexcept
    {
        resources = std::move(other.resources);
        solver = std::move(other.solver);
        solver.BindResources(resources);

        return *this;
    }

    blitzar_solvers::SolverTreeResources resources;
    blitzar_fmm::FmmSolver solver;
};

struct PmSolverBundle final {
    PmSolverBundle(blitzar_physics::GravityParameters gravity, std::size_t capacity)
        : solver(gravity, {{8, 8, 8}, capacity == 0 ? std::size_t{1} : capacity, 1.0}, capacity)
    {
    }

    PmSolverBundle(const PmSolverBundle&) = delete;
    PmSolverBundle& operator=(const PmSolverBundle&) = delete;
    PmSolverBundle(PmSolverBundle&&) noexcept = default;
    PmSolverBundle& operator=(PmSolverBundle&&) noexcept = default;

    blitzar_pm::PmSolver solver;
};

struct TreePmSolverBundle final {
    TreePmSolverBundle(blitzar_physics::GravityParameters gravity,
        blitzar_barnes_hut::BarnesHutSettings settings, std::size_t capacity)
        : resources({capacity == 0 ? std::size_t{1} : capacity, settings.max_cells,
                        settings.leaf_capacity, settings.max_depth},
              {settings.max_particles, settings.max_cells, settings.leaf_capacity,
                  settings.max_depth}),
          solver(blitzar_pm::PmSolver(gravity,
                     {{8, 8, 8}, capacity == 0 ? std::size_t{1} : capacity, 1.0}, capacity),
              blitzar_barnes_hut::BhSolver(gravity, settings, capacity, resources), resources)
    {
    }

    TreePmSolverBundle(const TreePmSolverBundle&) = delete;
    TreePmSolverBundle& operator=(const TreePmSolverBundle&) = delete;
    TreePmSolverBundle(TreePmSolverBundle&& other) noexcept
        : resources(std::move(other.resources)), solver(std::move(other.solver))
    {
        solver.BindResources(resources);
    }
    TreePmSolverBundle& operator=(TreePmSolverBundle&& other) noexcept
    {
        resources = std::move(other.resources);
        solver = std::move(other.solver);
        solver.BindResources(resources);

        return *this;
    }

    blitzar_solvers::SolverTreeResources resources;
    blitzar_treepm::TreePmSolver solver;
};

using SolverVariant = std::variant<DirectSolverBundle, BarnesHutSolverBundle, FmmSolverBundle,
    PmSolverBundle, TreePmSolverBundle>;

} // namespace blitzar_sim

#endif
