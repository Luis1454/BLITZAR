#ifndef BLITZAR_SOLVERS_SOLVER_FORCE_REQUEST_HPP
#define BLITZAR_SOLVERS_SOLVER_FORCE_REQUEST_HPP

#include "grid/GridResource.hpp"
#include "solvers/SolverContract.hpp"
#include "solvers/SolverForceEvaluation.hpp"
#include "trees/octree/OctreeResource.hpp"

namespace blitzar_solvers {

struct SolverForceRequest final {
    struct Direct final {
        blitzar_core::ParticleStateView targets;
        blitzar_core::ParticleStateView sources;
        blitzar_core::ForceView forces;
        const blitzar_core::ExecutionSettings& settings;
        ForceRange range;
        bool skip_self{false};
    };

    struct Tree final {
        blitzar_core::ParticleStateView targets;
        blitzar_core::ParticleStateView sources;
        blitzar_core::ForceView forces;
        const blitzar_core::ExecutionSettings& settings;
        const blitzar_trees::OctreeResource& resource;
        blitzar_trees::OctreeView tree;
        SolverForceSourceKind source_kind{SolverForceSourceKind::Local};
        bool accumulate{false};
        bool skip_self{false};
    };

    struct Grid final {
        blitzar_core::ParticleStateView targets;
        blitzar_core::ParticleStateView sources;
        blitzar_core::ForceView forces;
        const blitzar_core::ExecutionSettings& settings;
        const blitzar_grid::GridResource& resource;
        blitzar_grid::GridView grid;
        SolverForceSourceKind source_kind{SolverForceSourceKind::Local};
        bool accumulate{false};
        bool skip_self{false};
    };

    struct TreePm final {
        blitzar_core::ParticleStateView targets;
        blitzar_core::ParticleStateView sources;
        blitzar_core::ForceView forces;
        const blitzar_core::ExecutionSettings& settings;
        const blitzar_grid::GridResource& grid_resource;
        blitzar_grid::GridView grid;
        const blitzar_trees::OctreeResource& tree_resource;
        blitzar_trees::OctreeView tree;
        SolverForceSourceKind source_kind{SolverForceSourceKind::Local};
        bool accumulate{false};
        bool skip_self{false};
    };
};

} // namespace blitzar_solvers

#endif
