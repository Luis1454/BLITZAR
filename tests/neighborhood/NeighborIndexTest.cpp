#include "physics/neighbors/NeighborIndex.hpp"

#include "fixtures/FixtureCheck.hpp"
#include "particles/buffer/ParticleBuffer.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace {

constexpr std::size_t kCount = 128;

double DeterministicJitter(std::size_t index)
{
    std::uint64_t state = static_cast<std::uint64_t>(index + 1U) * 0x9E3779B97F4A7C15ULL;

    state = (state ^ (state >> 30U)) * 0xBF58476D1CE4E5B9ULL;
    state = (state ^ (state >> 27U)) * 0x94D049BB133111EBULL;
    state ^= state >> 31U;

    constexpr double kMax = 18446744073709551616.0;

    return static_cast<double>(state) / kMax;
}

void SeedLattice(blitzar_particles::ParticleBuffer& particles, std::size_t count) noexcept
{
    for (std::size_t index = 0; index < count; ++index) {
        const std::size_t ix = index % 8U;
        const std::size_t iy = (index / 8U) % 8U;
        const std::size_t iz = (index / 64U) % 8U;

        const blitzar_core::Scalar x = -2.0 + 0.5 * static_cast<blitzar_core::Scalar>(ix) +
                                       0.1 * (DeterministicJitter(index) - 0.5);

        const blitzar_core::Scalar y = -2.0 + 0.5 * static_cast<blitzar_core::Scalar>(iy) +
                                       0.1 * (DeterministicJitter(index + 101U) - 0.5);

        const blitzar_core::Scalar z = -2.0 + 0.5 * static_cast<blitzar_core::Scalar>(iz) +
                                       0.1 * (DeterministicJitter(index + 211U) - 0.5);

        (void)particles.SetPosition(index, {x, y, z});
    }
}

void ReferenceNeighbors(const blitzar_core::ParticleStateView& view, blitzar_core::Scalar radius,
    std::vector<std::vector<std::size_t>>& result)
{
    const blitzar_core::Scalar radius_squared = radius * radius;

    result.assign(view.count, {});

    for (std::size_t target = 0; target < view.count; ++target) {
        for (std::size_t source = 0; source < view.count; ++source) {
            if (source == target) {
                continue;
            }

            const blitzar_core::Scalar dx = view.x[target] - view.x[source];
            const blitzar_core::Scalar dy = view.y[target] - view.y[source];
            const blitzar_core::Scalar dz = view.z[target] - view.z[source];

            if (dx * dx + dy * dy + dz * dz <= radius_squared) {
                result[target].push_back(source);
            }
        }
    }
}

bool MatchesReference(const blitzar_physics::NeighborIndex& index,
    const std::vector<std::vector<std::size_t>>& reference, std::size_t count)
{
    for (std::size_t target = 0; target < count; ++target) {
        const blitzar_physics::NeighborSpan span = index.Query(target);

        if (span.indices.size() != reference[target].size()) {
            return false;
        }

        for (std::size_t offset = 0; offset < span.indices.size(); ++offset) {
            if (span.indices[offset] != reference[target][offset]) {
                return false;
            }
        }
    }

    return true;
}

bool IsSorted(const blitzar_physics::NeighborIndex& index, std::size_t count)
{
    for (std::size_t target = 0; target < count; ++target) {
        const blitzar_physics::NeighborSpan span = index.Query(target);

        for (std::size_t offset = 1; offset < span.indices.size(); ++offset) {
            if (span.indices[offset - 1U] >= span.indices[offset]) {
                return false;
            }
        }
    }

    return true;
}

bool SameResults(const blitzar_physics::NeighborIndex& first,
    const blitzar_physics::NeighborIndex& second, std::size_t count)
{
    for (std::size_t target = 0; target < count; ++target) {
        const blitzar_physics::NeighborSpan left = first.Query(target);
        const blitzar_physics::NeighborSpan right = second.Query(target);

        if (left.indices.size() != right.indices.size()) {
            return false;
        }

        for (std::size_t offset = 0; offset < left.indices.size(); ++offset) {
            if (left.indices[offset] != right.indices[offset]) {
                return false;
            }
        }
    }

    return true;
}

int CheckBruteForceAgreement() noexcept
{
    blitzar_particles::ParticleBuffer particles(kCount);

    SeedLattice(particles, kCount);

    const blitzar_core::ParticleStateView view = particles.State();
    const blitzar_physics::NeighborParameters parameters{
        0.5, 0.0, 512, 256, {-3.0, -3.0, -3.0}, {3.0, 3.0, 3.0}};

    blitzar_physics::NeighborIndex index(parameters);

    BLITZAR_CHECK(index.IsValid());
    BLITZAR_CHECK(index.Build(view.x, view.y, view.z) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(index.IsBuilt());

    std::vector<std::vector<std::size_t>> reference;

    ReferenceNeighbors(view, 0.5, reference);

    BLITZAR_CHECK(MatchesReference(index, reference, kCount));

    return 0;
}

int CheckDeterministicOrdering() noexcept
{
    blitzar_particles::ParticleBuffer particles(kCount);

    SeedLattice(particles, kCount);

    const blitzar_core::ParticleStateView view = particles.State();
    const blitzar_physics::NeighborParameters parameters{
        0.5, 0.0, 512, 256, {-3.0, -3.0, -3.0}, {3.0, 3.0, 3.0}};

    blitzar_physics::NeighborIndex first(parameters);
    blitzar_physics::NeighborIndex second(parameters);

    BLITZAR_CHECK(first.Build(view.x, view.y, view.z) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(second.Build(view.x, view.y, view.z) == BLITZAR_STATUS_OK);

    BLITZAR_CHECK(IsSorted(first, kCount));
    BLITZAR_CHECK(SameResults(first, second, kCount));

    return 0;
}

int CheckOverflowBeforeMutation() noexcept
{
    blitzar_particles::ParticleBuffer particles(30);

    for (std::size_t index = 0; index < 30; ++index) {
        BLITZAR_CHECK(particles.SetPosition(index, {0.0, 0.0, 0.0}) == BLITZAR_STATUS_OK);
    }

    const blitzar_core::ParticleStateView view = particles.State();
    const blitzar_physics::NeighborParameters overloaded{
        0.5, 0.0, 64, 8, {-2.0, -2.0, -2.0}, {2.0, 2.0, 2.0}};

    const blitzar_physics::NeighborParameters small_capacity{
        0.5, 0.0, 8, 64, {-0.75, -0.75, -0.75}, {0.75, 0.75, 0.75}};

    const blitzar_physics::NeighborParameters adequate{
        0.5, 0.0, 64, 64, {-0.75, -0.75, -0.75}, {0.75, 0.75, 0.75}};

    blitzar_physics::NeighborIndex neighbor_overloaded(overloaded);
    blitzar_physics::NeighborIndex particle_overloaded(small_capacity);
    blitzar_physics::NeighborIndex valid(adequate);

    BLITZAR_CHECK(
        neighbor_overloaded.Build(view.x, view.y, view.z) == BLITZAR_STATUS_ALLOCATION_FAILURE);

    BLITZAR_CHECK(
        particle_overloaded.Build(view.x, view.y, view.z) == BLITZAR_STATUS_ALLOCATION_FAILURE);

    BLITZAR_CHECK(!neighbor_overloaded.IsBuilt());
    BLITZAR_CHECK(!particle_overloaded.IsBuilt());

    BLITZAR_CHECK(valid.Build(view.x, view.y, view.z) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(valid.Count() == 30);

    return 0;
}

int CheckBoundaryAndExactRadius() noexcept
{
    blitzar_particles::ParticleBuffer particles(3);

    BLITZAR_CHECK(particles.SetPosition(0, {2.0, 0.0, 0.0}) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(particles.SetPosition(1, {1.5, 0.0, 0.0}) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(particles.SetPosition(2, {-2.0, -2.0, -2.0}) == BLITZAR_STATUS_OK);

    const blitzar_core::ParticleStateView view = particles.State();
    const blitzar_physics::NeighborParameters parameters{
        0.5, 0.0, 32, 32, {-2.0, -2.0, -2.0}, {2.0, 2.0, 2.0}};

    blitzar_physics::NeighborIndex index(parameters);

    BLITZAR_CHECK(index.Build(view.x, view.y, view.z) == BLITZAR_STATUS_OK);

    BLITZAR_CHECK(index.Query(0).indices.size() == 1);
    BLITZAR_CHECK(index.Query(0).indices[0] == 1);

    BLITZAR_CHECK(index.Query(1).indices.size() == 1);
    BLITZAR_CHECK(index.Query(1).indices[0] == 0);

    BLITZAR_CHECK(index.Query(2).indices.size() == 0);

    return 0;
}

int CheckEmptyAndValidation() noexcept
{
    const std::vector<blitzar_core::Scalar> empty_x{};
    const std::vector<blitzar_core::Scalar> empty_y{};
    const std::vector<blitzar_core::Scalar> empty_z{};

    const blitzar_physics::NeighborParameters parameters{
        0.5, 0.0, 32, 32, {-2.0, -2.0, -2.0}, {2.0, 2.0, 2.0}};

    blitzar_physics::NeighborIndex index(parameters);

    BLITZAR_CHECK(index.Build(empty_x, empty_y, empty_z) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(index.Count() == 0);
    BLITZAR_CHECK(index.Query(0).indices.size() == 0);

    const std::vector<blitzar_core::Scalar> short_y{0.0};

    BLITZAR_CHECK(index.Build(empty_x, short_y, empty_z) == BLITZAR_STATUS_INVALID_ARGUMENT);

    const blitzar_physics::NeighborParameters invalid_parameters{
        0.5, 0.0, 0, 16, {-2.0, -2.0, -2.0}, {2.0, 2.0, 2.0}};

    blitzar_physics::NeighborIndex invalid_index(invalid_parameters);

    BLITZAR_CHECK(!invalid_index.IsValid());
    BLITZAR_CHECK(
        invalid_index.Build(empty_x, empty_y, empty_z) == BLITZAR_STATUS_INVALID_ARGUMENT);

    const std::vector<blitzar_core::Scalar> x{0.0};
    const std::vector<blitzar_core::Scalar> y{0.0};
    const std::vector<blitzar_core::Scalar> z{
        std::numeric_limits<blitzar_core::Scalar>::quiet_NaN()};

    BLITZAR_CHECK(index.Build(x, y, z) == BLITZAR_STATUS_INVALID_ARGUMENT);

    return 0;
}

} // namespace

int main()
{
    BLITZAR_CHECK(CheckBruteForceAgreement() == 0);
    BLITZAR_CHECK(CheckDeterministicOrdering() == 0);
    BLITZAR_CHECK(CheckOverflowBeforeMutation() == 0);
    BLITZAR_CHECK(CheckBoundaryAndExactRadius() == 0);
    BLITZAR_CHECK(CheckEmptyAndValidation() == 0);

    return 0;
}
