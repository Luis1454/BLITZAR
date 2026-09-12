#include "trees/octree/resource/OctreeResource.hpp"

#include "fixtures/FixtureCheck.hpp"
#include "particles/buffer/ParticleBuffer.hpp"

#include <array>

int main()
{
    blitzar_particles::ParticleBuffer particles(3);

    BLITZAR_CHECK(particles.SetPosition(0, {-1.0, 0.0, 0.0}) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(particles.SetPosition(1, {0.0, 1.0, 0.0}) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(particles.SetPosition(2, {1.0, 0.0, 1.0}) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(particles.SetMass(0, 1.0) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(particles.SetMass(1, 2.0) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(particles.SetMass(2, 3.0) == BLITZAR_STATUS_OK);

    blitzar_trees::OctreeResource resource({3, 64, 1, 8});

    BLITZAR_CHECK(resource.View().IsValid() == false);
    BLITZAR_CHECK(resource.Prepare(particles.State()) == BLITZAR_STATUS_OK);

    const blitzar_trees::OctreeView first = resource.View();

    BLITZAR_CHECK(first.IsValid());
    BLITZAR_CHECK(resource.IsCurrent(first));
    BLITZAR_CHECK(first.ParticleCount() == 3);
    BLITZAR_CHECK(first.Cells().size() > 0);
    BLITZAR_CHECK(first.Indices().size() == 3);
    BLITZAR_CHECK(resource.BuildCount() == 1);

    BLITZAR_CHECK(particles.SetPosition(0, {-0.9, 0.0, 0.0}) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(resource.Prepare(particles.State()) == BLITZAR_STATUS_OK);

    const blitzar_trees::OctreeView second = resource.View();

    BLITZAR_CHECK(second.IsValid());
    BLITZAR_CHECK(resource.IsCurrent(second));
    BLITZAR_CHECK(!resource.IsCurrent(first));
    BLITZAR_CHECK(second.Generation() != first.Generation());
    BLITZAR_CHECK(resource.RefitCount() == 1);

    std::array<double, 4> x{-1.0, 0.0, 1.0, 2.0};
    std::array<double, 4> y{};
    std::array<double, 4> z{};
    std::array<double, 4> velocity_x{};
    std::array<double, 4> velocity_y{};
    std::array<double, 4> velocity_z{};
    std::array<double, 4> mass{1.0, 1.0, 1.0, 1.0};
    const blitzar_core::ParticleStateView oversized{
        4, x, y, z, velocity_x, velocity_y, velocity_z, mass};

    BLITZAR_CHECK(resource.Prepare(oversized) == BLITZAR_STATUS_INVALID_ARGUMENT);
    BLITZAR_CHECK(resource.IsCurrent(second));

    const std::array<double, 3> empty{};
    const blitzar_core::ParticleStateView invalid{
        0, empty, empty, empty, empty, empty, empty, empty};

    BLITZAR_CHECK(resource.Prepare(invalid) == BLITZAR_STATUS_INVALID_ARGUMENT);
    BLITZAR_CHECK(resource.IsCurrent(second));

    return 0;
}
