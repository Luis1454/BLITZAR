#include "fixtures/FixtureCheck.hpp"
#include "particles/buffer/ParticleBuffer.hpp"
#include "trees/octree/Octree.hpp"

#include <cstddef>

int main()
{
    blitzar_particles::ParticleBuffer particles(4);
    const blitzar_core::Vector3 positions[] = {
        {-1.0, -1.0, -1.0}, {1.0, -1.0, 1.0}, {-1.0, 1.0, 1.0}, {1.0, 1.0, -1.0}};

    for (std::size_t index = 0; index < 4; ++index) {
        BLITZAR_CHECK(particles.SetPosition(index, positions[index]) == BLITZAR_STATUS_OK);
        BLITZAR_CHECK(particles.SetMass(index, 1.0) == BLITZAR_STATUS_OK);
    }

    blitzar_trees::Octree tree(4, 128, 1, 8);

    BLITZAR_CHECK(tree.Build(particles.State()) == BLITZAR_STATUS_OK);

    const blitzar_trees::OctreeView view = tree.View();

    BLITZAR_CHECK(view.IsValid());
    BLITZAR_CHECK(tree.IsCurrent(view));
    BLITZAR_CHECK(view.ParticleCount() == 4);
    BLITZAR_CHECK(view.Cells().size() == tree.CellCount());
    BLITZAR_CHECK(view.Indices().size() == 4);
    BLITZAR_CHECK(view.CellAt(view.Cells().size()).empty());

    for (std::size_t index = 0; index < view.Indices().size(); ++index) {
        std::size_t particle = 0;

        BLITZAR_CHECK(view.ParticleIndex(index, particle));
        BLITZAR_CHECK(particle < 4);
    }

    std::size_t invalid_particle = 0;

    BLITZAR_CHECK(!view.ParticleIndex(view.Indices().size(), invalid_particle));

    BLITZAR_CHECK(particles.SetPosition(0, {-0.9, -1.0, -1.0}) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(tree.Refit(particles.State()));
    BLITZAR_CHECK(!tree.IsCurrent(view));
    BLITZAR_CHECK(tree.View().IsValid());

    return 0;
}
