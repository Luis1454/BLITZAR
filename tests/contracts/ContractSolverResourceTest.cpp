#include "fixtures/FixtureCheck.hpp"
#include "solvers/SolverContract.hpp"

#include <blitzar/blitzar.h>
#include <type_traits>

using blitzar_solvers::SolverKind;
using blitzar_solvers::SolverResourceContract;
using blitzar_solvers::SolverWorkspaceKind;
using blitzar_solvers::SpatialResourceKind;

static_assert(std::is_aggregate_v<SolverResourceContract>);
static_assert(SolverResourceContract::For(SolverKind::Direct).IsValid());
static_assert(SolverResourceContract::For(SolverKind::Kifmm).RequiresOctree());
static_assert(SolverResourceContract::For(SolverKind::Kifmm).RequiresPrivateWorkspace());

int main()
{
    const SolverResourceContract direct = SolverResourceContract::For(SolverKind::Direct);
    const SolverResourceContract barnes_hut = SolverResourceContract::For(SolverKind::BarnesHut);

    const SolverResourceContract fmm = SolverResourceContract::For(SolverKind::Fmm);
    const SolverResourceContract kifmm = SolverResourceContract::For(SolverKind::Kifmm);
    const SolverResourceContract pm = SolverResourceContract::For(SolverKind::Pm);
    const SolverResourceContract tree_pm = SolverResourceContract::For(SolverKind::TreePm);

    BLITZAR_CHECK(direct.IsValid());
    BLITZAR_CHECK(direct.spatial == SpatialResourceKind::None);
    BLITZAR_CHECK(!direct.RequiresOctree());
    BLITZAR_CHECK(!direct.RequiresGrid());

    BLITZAR_CHECK(barnes_hut.IsValid());
    BLITZAR_CHECK(barnes_hut.spatial == SpatialResourceKind::Octree);
    BLITZAR_CHECK(barnes_hut.RequiresOctree());
    BLITZAR_CHECK(!barnes_hut.RequiresGrid());

    BLITZAR_CHECK(fmm.IsValid());
    BLITZAR_CHECK(fmm.spatial == SpatialResourceKind::Octree);
    BLITZAR_CHECK(fmm.workspace == SolverWorkspaceKind::None);

    BLITZAR_CHECK(kifmm.IsValid());
    BLITZAR_CHECK(kifmm.spatial == SpatialResourceKind::Octree);
    BLITZAR_CHECK(kifmm.workspace == SolverWorkspaceKind::KernelIndependentMultipole);
    BLITZAR_CHECK(kifmm.RequiresPrivateWorkspace());

    BLITZAR_CHECK(pm.IsValid());
    BLITZAR_CHECK(pm.spatial == SpatialResourceKind::Grid);
    BLITZAR_CHECK(pm.RequiresGrid());
    BLITZAR_CHECK(!pm.RequiresOctree());

    BLITZAR_CHECK(tree_pm.IsValid());
    BLITZAR_CHECK(tree_pm.spatial == SpatialResourceKind::OctreeAndGrid);
    BLITZAR_CHECK(tree_pm.RequiresOctree());
    BLITZAR_CHECK(tree_pm.RequiresGrid());

    const SolverResourceContract invalid{
        SolverKind::Direct, SpatialResourceKind::Octree, SolverWorkspaceKind::None};

    const SolverResourceContract unknown =
        SolverResourceContract::For(static_cast<SolverKind>(255));

    BLITZAR_CHECK(!invalid.IsValid());
    BLITZAR_CHECK(!invalid.RequiresPrivateWorkspace());
    BLITZAR_CHECK(!unknown.IsValid());
    BLITZAR_CHECK(!unknown.RequiresOctree());
    BLITZAR_CHECK(!unknown.RequiresGrid());
    BLITZAR_CHECK(BLITZAR_SOLVER_TREEPM == 4);

    return 0;
}
