#ifndef BLITZAR_SOLVERS_SOLVER_TREE_RESOURCES_HPP
#define BLITZAR_SOLVERS_SOLVER_TREE_RESOURCES_HPP

#include "trees/octree/OctreeResource.hpp"

namespace blitzar_solvers {

class SolverTreeResources final {
public:
    SolverTreeResources(
        blitzar_trees::OctreeResourceConfig local, blitzar_trees::OctreeResourceConfig remote);

    SolverTreeResources(const SolverTreeResources&) = delete;
    SolverTreeResources& operator=(const SolverTreeResources&) = delete;
    SolverTreeResources(SolverTreeResources&&) noexcept = default;
    SolverTreeResources& operator=(SolverTreeResources&&) noexcept = default;

    [[nodiscard]] blitzar_trees::OctreeResource& Local() noexcept;
    [[nodiscard]] const blitzar_trees::OctreeResource& Local() const noexcept;
    [[nodiscard]] blitzar_trees::OctreeResource& Remote() noexcept;
    [[nodiscard]] const blitzar_trees::OctreeResource& Remote() const noexcept;
    void ReplaceLocal(blitzar_trees::OctreeResource resource) noexcept;

private:
    blitzar_trees::OctreeResource local_;
    blitzar_trees::OctreeResource remote_;
};

} // namespace blitzar_solvers

#endif
