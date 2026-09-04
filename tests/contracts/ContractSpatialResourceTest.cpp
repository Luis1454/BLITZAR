#include "fixtures/FixtureCheck.hpp"
#include "particles/buffer/ParticleBuffer.hpp"
#include "simulation/solver/SimSolverVariant.hpp"
#include "solvers/SolverTreeResources.hpp"
#include "trees/octree/OctreeResource.hpp"

#include <blitzar/blitzar.h>
#include <cstddef>
#include <type_traits>
#include <utility>
#include <variant>

static_assert(std::variant_size_v<blitzar_sim::SolverVariant> == 5);
static_assert(!std::is_copy_constructible_v<blitzar_solvers::SolverTreeResources>);
static_assert(std::is_move_constructible_v<blitzar_solvers::SolverTreeResources>);

namespace {

int InitializeParticles(blitzar_particles::ParticleBuffer& particles)
{
    for (std::size_t index = 0; index < particles.Count(); ++index) {
        BLITZAR_CHECK(particles.SetPosition(index, {static_cast<double>(index), 0.0, 0.0}) ==
                      BLITZAR_STATUS_OK);

        BLITZAR_CHECK(particles.SetMass(index, 1.0) == BLITZAR_STATUS_OK);
    }

    return 0;
}

int VerifyInitialView(const blitzar_trees::OctreeResource& resource)
{
    const blitzar_trees::OctreeView view = resource.View();

    BLITZAR_CHECK(!view.IsValid());
    BLITZAR_CHECK(view.Generation() == 0);
    BLITZAR_CHECK(!resource.IsCurrent(view));

    return 0;
}

int PrepareAndVerify(
    blitzar_particles::ParticleBuffer& particles, blitzar_trees::OctreeResource& resource)
{
    BLITZAR_CHECK(resource.Prepare(particles.State()) == BLITZAR_STATUS_OK);

    const blitzar_trees::OctreeView view = resource.View();

    BLITZAR_CHECK(view.IsValid());
    BLITZAR_CHECK(resource.IsCurrent(view));
    BLITZAR_CHECK(view.ParticleCount() == 3);
    BLITZAR_CHECK(!view.Cells().empty());
    BLITZAR_CHECK(view.Indices().size() == 3);
    BLITZAR_CHECK(view.CellAt(32).empty());

    for (std::size_t index = 0; index < view.Indices().size(); ++index) {
        std::size_t particle = 0;

        BLITZAR_CHECK(view.ParticleIndex(index, particle));
        BLITZAR_CHECK(particle < 3);
    }

    return 0;
}

int RejectOversized(blitzar_trees::OctreeResource& resource,
    const blitzar_particles::ParticleBuffer& oversized_particles)
{
    const blitzar_trees::OctreeView view = resource.View();

    BLITZAR_CHECK(resource.Prepare(oversized_particles.State()) == BLITZAR_STATUS_INVALID_ARGUMENT);
    BLITZAR_CHECK(resource.IsCurrent(view));

    return 0;
}

int RefitAndVerify(
    blitzar_particles::ParticleBuffer& particles, blitzar_trees::OctreeResource& resource)
{
    const blitzar_trees::OctreeView built = resource.View();

    BLITZAR_CHECK(particles.SetPosition(1, {1.25, 0.0, 0.0}) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(resource.Prepare(particles.State()) == BLITZAR_STATUS_OK);

    const blitzar_trees::OctreeView refitted = resource.View();

    BLITZAR_CHECK(refitted.Generation() != built.Generation());
    BLITZAR_CHECK(!resource.IsCurrent(built));
    BLITZAR_CHECK(resource.IsCurrent(refitted));
    BLITZAR_CHECK(resource.RefitCount() == 1);

    return 0;
}

int MoveAndVerify(blitzar_trees::OctreeResource& resource)
{
    const blitzar_trees::OctreeView view = resource.View();
    blitzar_trees::OctreeResource moved(std::move(resource));

    BLITZAR_CHECK(!moved.IsCurrent(view));
    BLITZAR_CHECK(moved.IsCurrent(moved.View()));

    return 0;
}

int VerifyBundle(const blitzar_trees::OctreeResourceConfig& config,
    const blitzar_particles::ParticleBuffer& particles)
{
    blitzar_solvers::SolverTreeResources bundle(config, config);

    BLITZAR_CHECK(bundle.Local().Prepare(particles.State()) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(bundle.Remote().Prepare(particles.State()) == BLITZAR_STATUS_OK);

    const blitzar_trees::OctreeView local = bundle.Local().View();

    BLITZAR_CHECK(bundle.Local().IsCurrent(local));
    BLITZAR_CHECK(bundle.Remote().IsCurrent(bundle.Remote().View()));

    blitzar_trees::OctreeResource replacement(config);

    BLITZAR_CHECK(replacement.Prepare(particles.State()) == BLITZAR_STATUS_OK);

    bundle.ReplaceLocal(std::move(replacement));

    BLITZAR_CHECK(!bundle.Local().IsCurrent(local));

    return 0;
}

int RunResourceChecks()
{
    blitzar_particles::ParticleBuffer particles(3);

    BLITZAR_CHECK(InitializeParticles(particles) == 0);

    const blitzar_trees::OctreeResourceConfig config{3, 32, 1, 8};
    blitzar_trees::OctreeResource resource(config);

    BLITZAR_CHECK(VerifyInitialView(resource) == 0);
    BLITZAR_CHECK(PrepareAndVerify(particles, resource) == 0);

    blitzar_particles::ParticleBuffer oversized_particles(4);

    BLITZAR_CHECK(RejectOversized(resource, oversized_particles) == 0);
    BLITZAR_CHECK(RefitAndVerify(particles, resource) == 0);
    BLITZAR_CHECK(MoveAndVerify(resource) == 0);

    return 0;
}

int RunBundleChecks()
{
    blitzar_particles::ParticleBuffer particles(3);
    const blitzar_trees::OctreeResourceConfig config{3, 32, 1, 8};

    BLITZAR_CHECK(InitializeParticles(particles) == 0);
    BLITZAR_CHECK(VerifyBundle(config, particles) == 0);

    return 0;
}

} // namespace

int main()
{
    return RunResourceChecks() + RunBundleChecks();
}
