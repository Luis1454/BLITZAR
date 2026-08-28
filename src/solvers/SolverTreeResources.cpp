#include "solvers/SolverTreeResources.hpp"

#include <utility>

namespace blitzar_solvers {

SolverTreeResources::SolverTreeResources(
    blitzar_trees::OctreeResourceConfig local, blitzar_trees::OctreeResourceConfig remote)
    : local_(local), remote_(remote)
{
}

blitzar_trees::OctreeResource& SolverTreeResources::Local() noexcept
{
    return local_;
}

const blitzar_trees::OctreeResource& SolverTreeResources::Local() const noexcept
{
    return local_;
}

blitzar_trees::OctreeResource& SolverTreeResources::Remote() noexcept
{
    return remote_;
}

const blitzar_trees::OctreeResource& SolverTreeResources::Remote() const noexcept
{
    return remote_;
}

void SolverTreeResources::ReplaceLocal(blitzar_trees::OctreeResource resource) noexcept
{
    local_ = std::move(resource);
}

} // namespace blitzar_solvers
