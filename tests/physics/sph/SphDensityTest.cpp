#include "physics/sph/SphDensity.hpp"

#include "fixtures/FixtureCheck.hpp"
#include "particles/buffer/ParticleBuffer.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace {

constexpr double kPi = 3.141592653589793238462643383279502884;

bool Near(blitzar_core::Scalar left, blitzar_core::Scalar right, blitzar_core::Scalar relative)
{
    const blitzar_core::Scalar scale = std::max(std::abs(left), std::abs(right));

    return std::abs(left - right) <= relative * scale;
}

void Put(blitzar_particles::ParticleBuffer& particles, std::size_t index, blitzar_core::Scalar px,
    blitzar_core::Scalar py, blitzar_core::Scalar pz)
{
    (void)particles.SetPosition(index, {px, py, pz});
}

blitzar_physics::NeighborIndex BuildIndex(blitzar_particles::ParticleBuffer& particles,
    const blitzar_physics::NeighborParameters& parameters)
{
    blitzar_physics::NeighborIndex index(parameters);
    const blitzar_core::ParticleStateView view = particles.State();

    (void)index.Build(view.x, view.y, view.z);

    return index;
}

int CheckKernelNormalization() noexcept
{
    const blitzar_core::Scalar inverse_pi = blitzar_core::Scalar{1} / kPi;
    const blitzar_core::Scalar center = blitzar_physics::SphDensity::Weight(0.0, 1.0);
    const blitzar_core::Scalar halfway = blitzar_physics::SphDensity::Weight(0.5, 1.0);
    const blitzar_core::Scalar support_edge = blitzar_physics::SphDensity::Weight(1.0, 1.0);
    const blitzar_core::Scalar in_tail = blitzar_physics::SphDensity::Weight(1.5, 1.0);
    const blitzar_core::Scalar outside = blitzar_physics::SphDensity::Weight(2.0, 1.0);

    BLITZAR_CHECK(Near(center, inverse_pi, blitzar_core::Scalar{1e-12}));
    BLITZAR_CHECK(Near(halfway,
        inverse_pi * (blitzar_core::Scalar{1} - blitzar_core::Scalar{1.5} * 0.25 +
                         blitzar_core::Scalar{0.75} * 0.125),
        blitzar_core::Scalar{1e-12}));

    BLITZAR_CHECK(
        Near(support_edge, blitzar_core::Scalar{0.25} * inverse_pi, blitzar_core::Scalar{1e-12}));

    BLITZAR_CHECK(
        Near(in_tail, blitzar_core::Scalar{0.25} * inverse_pi * blitzar_core::Scalar{0.125},
            blitzar_core::Scalar{1e-12}));

    BLITZAR_CHECK(outside == blitzar_core::Scalar{0});
    BLITZAR_CHECK(blitzar_physics::SphDensity::Weight(1.0, 0.0) == blitzar_core::Scalar{0});
    BLITZAR_CHECK(blitzar_core::Scalar{0.25} * inverse_pi * blitzar_core::Scalar{0.25} > outside);

    blitzar_core::Scalar previous = blitzar_physics::SphDensity::Weight(0.0, 1.0);

    for (std::size_t step = 1; step <= 20; ++step) {
        const blitzar_core::Scalar q = blitzar_core::Scalar(step) * blitzar_core::Scalar{0.1};
        const blitzar_core::Scalar current = blitzar_physics::SphDensity::Weight(q, 1.0);

        BLITZAR_CHECK(current <= previous);

        previous = current;
    }

    return 0;
}

int CheckTwoBodyExact() noexcept
{
    constexpr blitzar_core::Scalar kMass = blitzar_core::Scalar{3.0};
    constexpr blitzar_core::Scalar kSmooth = blitzar_core::Scalar{0.8};
    constexpr blitzar_core::Scalar kSeparation = blitzar_core::Scalar{0.6};

    blitzar_particles::ParticleBuffer particles(2);

    Put(particles, 0, 0.0, 0.0, 0.0);
    Put(particles, 1, kSeparation, 0.0, 0.0);

    const blitzar_physics::NeighborParameters two_parameters{
        blitzar_core::Scalar{2} * kSmooth, 0.0, 8, 8, {-2.0, -2.0, -2.0}, {2.0, 2.0, 2.0}};

    const blitzar_physics::NeighborIndex index = BuildIndex(particles, two_parameters);

    const std::vector<blitzar_core::Scalar> mass{kMass, kMass};
    const std::vector<blitzar_core::Scalar> smooth{kSmooth, kSmooth};
    const std::vector<blitzar_core::Scalar> raw_expected{
        kMass * (blitzar_physics::SphDensity::Weight(0.0, kSmooth) +
                    blitzar_physics::SphDensity::Weight(kSeparation, kSmooth)),
        kMass * (blitzar_physics::SphDensity::Weight(0.0, kSmooth) +
                    blitzar_physics::SphDensity::Weight(kSeparation, kSmooth))};

    const blitzar_core::ParticleStateView view = particles.State();
    const blitzar_physics::SphDensityRequest raw_request{
        index, view.x, view.y, view.z, mass, smooth};

    std::vector<blitzar_core::Scalar> raw_density(2);
    blitzar_physics::SphDensity raw_sph({false});

    BLITZAR_CHECK(raw_sph.Evaluate(raw_request, raw_density) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(Near(raw_density[0], raw_expected[0], blitzar_core::Scalar{1e-12}));
    BLITZAR_CHECK(Near(raw_density[1], raw_expected[1], blitzar_core::Scalar{1e-12}));

    const blitzar_physics::SphDensityRequest normalized_request{
        index, view.x, view.y, view.z, mass, smooth};

    std::vector<blitzar_core::Scalar> normalized_density(2);
    blitzar_physics::SphDensity normalized_sph({true});

    BLITZAR_CHECK(
        normalized_sph.Evaluate(normalized_request, normalized_density) == BLITZAR_STATUS_OK);

    BLITZAR_CHECK(Near(normalized_density[0], kMass, blitzar_core::Scalar{1e-12}));
    BLITZAR_CHECK(Near(normalized_density[1], kMass, blitzar_core::Scalar{1e-12}));

    return 0;
}

int CheckUniformLatticeInterior() noexcept
{
    constexpr std::size_t kGrid = 11;
    constexpr blitzar_core::Scalar kCell = blitzar_core::Scalar{0.5};
    constexpr blitzar_core::Scalar kMass = kCell * kCell * kCell;
    constexpr blitzar_core::Scalar kSmooth = blitzar_core::Scalar{1.0};
    constexpr blitzar_core::Scalar kBox = blitzar_core::Scalar{2.5};

    blitzar_particles::ParticleBuffer particles(kGrid * kGrid * kGrid);

    for (std::size_t ix = 0; ix < kGrid; ++ix) {
        for (std::size_t iy = 0; iy < kGrid; ++iy) {
            for (std::size_t iz = 0; iz < kGrid; ++iz) {
                const std::size_t index = ix + kGrid * (iy + kGrid * iz);

                Put(particles, index, -kBox + kCell * blitzar_core::Scalar(ix),
                    -kBox + kCell * blitzar_core::Scalar(iy),
                    -kBox + kCell * blitzar_core::Scalar(iz));
            }
        }
    }

    const blitzar_physics::NeighborParameters lattice_parameters{
        blitzar_core::Scalar{2} * kSmooth, 0.0, 8192, 2048, {-3.0, -3.0, -3.0}, {3.0, 3.0, 3.0}};

    const blitzar_physics::NeighborIndex index = BuildIndex(particles, lattice_parameters);

    const std::vector<blitzar_core::Scalar> mass(kGrid * kGrid * kGrid, kMass);
    const std::vector<blitzar_core::Scalar> smooth(kGrid * kGrid * kGrid, kSmooth);

    const blitzar_core::ParticleStateView view = particles.State();
    const blitzar_physics::SphDensityRequest request{index, view.x, view.y, view.z, mass, smooth};

    std::vector<blitzar_core::Scalar> density(kGrid * kGrid * kGrid);
    blitzar_physics::SphDensity sph({false});

    BLITZAR_CHECK(sph.Evaluate(request, density) == BLITZAR_STATUS_OK);

    const blitzar_core::Scalar margin = kBox - blitzar_core::Scalar{2} * kSmooth;

    for (std::size_t ix = 0; ix < kGrid; ++ix) {
        for (std::size_t iy = 0; iy < kGrid; ++iy) {
            for (std::size_t iz = 0; iz < kGrid; ++iz) {
                const bool interior =
                    (std::abs(-kBox + kCell * blitzar_core::Scalar(ix)) <= margin) &&
                    (std::abs(-kBox + kCell * blitzar_core::Scalar(iy)) <= margin) &&
                    (std::abs(-kBox + kCell * blitzar_core::Scalar(iz)) <= margin);

                if (!interior) {
                    continue;
                }

                const std::size_t index = ix + kGrid * (iy + kGrid * iz);

                BLITZAR_CHECK(
                    Near(density[index], blitzar_core::Scalar{1.0}, blitzar_core::Scalar{1e-2}));
            }
        }
    }

    return 0;
}

int CheckShepardBoundary() noexcept
{
    constexpr std::size_t kGrid = 11;
    constexpr blitzar_core::Scalar kCell = blitzar_core::Scalar{0.5};
    constexpr blitzar_core::Scalar kMass = kCell * kCell * kCell;
    constexpr blitzar_core::Scalar kSmooth = blitzar_core::Scalar{1.0};
    constexpr blitzar_core::Scalar kBox = blitzar_core::Scalar{2.5};

    blitzar_particles::ParticleBuffer particles(kGrid * kGrid * kGrid);

    for (std::size_t ix = 0; ix < kGrid; ++ix) {
        for (std::size_t iy = 0; iy < kGrid; ++iy) {
            for (std::size_t iz = 0; iz < kGrid; ++iz) {
                const std::size_t index = ix + kGrid * (iy + kGrid * iz);

                Put(particles, index, -kBox + kCell * blitzar_core::Scalar(ix),
                    -kBox + kCell * blitzar_core::Scalar(iy),
                    -kBox + kCell * blitzar_core::Scalar(iz));
            }
        }
    }

    const blitzar_physics::NeighborParameters lattice_parameters{
        blitzar_core::Scalar{2} * kSmooth, 0.0, 8192, 2048, {-3.0, -3.0, -3.0}, {3.0, 3.0, 3.0}};

    const blitzar_physics::NeighborIndex index = BuildIndex(particles, lattice_parameters);

    const std::vector<blitzar_core::Scalar> mass(kGrid * kGrid * kGrid, kMass);
    const std::vector<blitzar_core::Scalar> smooth(kGrid * kGrid * kGrid, kSmooth);

    const blitzar_core::ParticleStateView view = particles.State();
    const blitzar_physics::SphDensityRequest request{index, view.x, view.y, view.z, mass, smooth};

    std::vector<blitzar_core::Scalar> density(kGrid * kGrid * kGrid);
    blitzar_physics::SphDensity sph({true});

    BLITZAR_CHECK(sph.Evaluate(request, density) == BLITZAR_STATUS_OK);

    for (std::size_t index = 0; index < kGrid * kGrid * kGrid; ++index) {
        BLITZAR_CHECK(Near(density[index], kMass, blitzar_core::Scalar{1e-10}));
    }

    return 0;
}

int CheckAnalyticField() noexcept
{
    constexpr std::size_t kGrid = 11;
    constexpr blitzar_core::Scalar kCell = blitzar_core::Scalar{0.5};
    constexpr blitzar_core::Scalar kSmooth = blitzar_core::Scalar{1.0};
    constexpr blitzar_core::Scalar kBox = blitzar_core::Scalar{2.5};

    auto analytic = [](blitzar_core::Scalar px) {
        const blitzar_core::Scalar period = blitzar_core::Scalar{2} * kBox;

        return blitzar_core::Scalar{1.0} +
               blitzar_core::Scalar{0.05} * std::sin(blitzar_core::Scalar{2} * kPi * px / period);
    };

    blitzar_particles::ParticleBuffer particles(kGrid * kGrid * kGrid);

    std::vector<blitzar_core::Scalar> mass(kGrid * kGrid * kGrid);

    for (std::size_t ix = 0; ix < kGrid; ++ix) {
        for (std::size_t iy = 0; iy < kGrid; ++iy) {
            for (std::size_t iz = 0; iz < kGrid; ++iz) {
                const std::size_t index = ix + kGrid * (iy + kGrid * iz);
                const blitzar_core::Scalar px = -kBox + kCell * blitzar_core::Scalar(ix);
                const blitzar_core::Scalar py = -kBox + kCell * blitzar_core::Scalar(iy);
                const blitzar_core::Scalar pz = -kBox + kCell * blitzar_core::Scalar(iz);

                Put(particles, index, px, py, pz);

                mass[index] = kCell * kCell * kCell * analytic(px);
            }
        }
    }

    const blitzar_physics::NeighborParameters lattice_parameters{
        blitzar_core::Scalar{2} * kSmooth, 0.0, 8192, 2048, {-3.0, -3.0, -3.0}, {3.0, 3.0, 3.0}};

    const blitzar_physics::NeighborIndex index = BuildIndex(particles, lattice_parameters);

    const std::vector<blitzar_core::Scalar> smooth(kGrid * kGrid * kGrid, kSmooth);

    const blitzar_core::ParticleStateView view = particles.State();
    const blitzar_physics::SphDensityRequest request{index, view.x, view.y, view.z, mass, smooth};

    std::vector<blitzar_core::Scalar> density(kGrid * kGrid * kGrid);
    blitzar_physics::SphDensity sph({false});

    BLITZAR_CHECK(sph.Evaluate(request, density) == BLITZAR_STATUS_OK);

    const blitzar_core::Scalar margin = kBox - blitzar_core::Scalar{2} * kSmooth;

    for (std::size_t ix = 0; ix < kGrid; ++ix) {
        const blitzar_core::Scalar px = -kBox + kCell * blitzar_core::Scalar(ix);
        const bool interior_x = std::abs(px) <= margin;

        if (!interior_x) {
            continue;
        }

        for (std::size_t iz = 0; iz < kGrid; ++iz) {
            const blitzar_core::Scalar pz = -kBox + kCell * blitzar_core::Scalar(iz);
            const bool interior_z = std::abs(pz) <= margin;

            if (!interior_z) {
                continue;
            }

            for (std::size_t iy = 0; iy < kGrid; ++iy) {
                const blitzar_core::Scalar py = -kBox + kCell * blitzar_core::Scalar(iy);
                const bool interior_y = std::abs(py) <= margin;

                if (!interior_y) {
                    continue;
                }

                const std::size_t index = ix + kGrid * (iy + kGrid * iz);

                BLITZAR_CHECK(Near(density[index], analytic(px), blitzar_core::Scalar{1e-2}));
            }
        }
    }

    return 0;
}

int CheckFailSafe() noexcept
{
    constexpr blitzar_core::Scalar kMass = blitzar_core::Scalar{3.0};
    constexpr blitzar_core::Scalar kSmooth = blitzar_core::Scalar{0.8};
    constexpr blitzar_core::Scalar kSentinel = blitzar_core::Scalar{1234.5};

    blitzar_particles::ParticleBuffer isolated(1);

    Put(isolated, 0, 0.0, 0.0, 0.0);

    const blitzar_physics::NeighborParameters isolated_parameters{
        blitzar_core::Scalar{2} * kSmooth, 0.0, 8, 8, {-2.0, -2.0, -2.0}, {2.0, 2.0, 2.0}};

    const blitzar_physics::NeighborIndex isolated_index = BuildIndex(isolated, isolated_parameters);

    const std::vector<blitzar_core::Scalar> one_mass{kMass};
    const std::vector<blitzar_core::Scalar> one_smooth{kSmooth};

    blitzar_core::ParticleStateView stripped_view = isolated.State();

    const blitzar_physics::SphDensityRequest isolated_request{
        isolated_index, stripped_view.x, stripped_view.y, stripped_view.z, one_mass, one_smooth};

    std::vector<blitzar_core::Scalar> isolated_density{kSentinel};
    blitzar_physics::SphDensity sph({false});

    BLITZAR_CHECK(sph.Evaluate(isolated_request, isolated_density) == BLITZAR_STATUS_SINGULARITY);
    BLITZAR_CHECK(isolated_density[0] == kSentinel);

    blitzar_particles::ParticleBuffer pair(2);

    Put(pair, 0, 0.0, 0.0, 0.0);
    Put(pair, 1, kSmooth * blitzar_core::Scalar{0.5}, 0.0, 0.0);

    const blitzar_physics::NeighborIndex pair_index = BuildIndex(pair, isolated_parameters);

    const blitzar_core::ParticleStateView pair_view = pair.State();
    const std::vector<blitzar_core::Scalar> zero_mass{0.0, 0.0};
    const std::vector<blitzar_core::Scalar> pair_smooth{kSmooth, kSmooth};
    const blitzar_physics::SphDensityRequest zero_request{
        pair_index, pair_view.x, pair_view.y, pair_view.z, zero_mass, pair_smooth};

    std::vector<blitzar_core::Scalar> zero_density{kSentinel, kSentinel};

    BLITZAR_CHECK(sph.Evaluate(zero_request, zero_density) == BLITZAR_STATUS_SINGULARITY);
    BLITZAR_CHECK(zero_density[0] == kSentinel);
    BLITZAR_CHECK(zero_density[1] == kSentinel);

    const std::vector<blitzar_core::Scalar> negative_mass{-1.0, kMass};
    const blitzar_physics::SphDensityRequest negative_request{
        pair_index, pair_view.x, pair_view.y, pair_view.z, negative_mass, pair_smooth};

    std::vector<blitzar_core::Scalar> negative_density{kSentinel, kSentinel};

    BLITZAR_CHECK(
        sph.Evaluate(negative_request, negative_density) == BLITZAR_STATUS_INVALID_ARGUMENT);

    BLITZAR_CHECK(negative_density[0] == kSentinel);

    const std::vector<blitzar_core::Scalar> zero_smooth{0.0, kSmooth};
    const blitzar_physics::SphDensityRequest smooth_request{
        pair_index, pair_view.x, pair_view.y, pair_view.z, one_mass, zero_smooth};

    std::vector<blitzar_core::Scalar> smooth_density{kSentinel, kSentinel};

    BLITZAR_CHECK(sph.Evaluate(smooth_request, smooth_density) == BLITZAR_STATUS_INVALID_ARGUMENT);
    BLITZAR_CHECK(smooth_density[0] == kSentinel);

    const blitzar_physics::SphDensityRequest mismatched_request{
        pair_index, pair_view.x, pair_view.y, pair_view.z, one_mass, one_smooth};

    std::vector<blitzar_core::Scalar> short_density{kSentinel};

    BLITZAR_CHECK(
        sph.Evaluate(mismatched_request, short_density) == BLITZAR_STATUS_INVALID_ARGUMENT);

    const blitzar_physics::NeighborParameters unbuilt_parameters{
        blitzar_core::Scalar{2} * kSmooth, 0.0, 8, 8, {-2.0, -2.0, -2.0}, {2.0, 2.0, 2.0}};

    blitzar_physics::NeighborIndex unbuilt(unbuilt_parameters);

    const blitzar_physics::SphDensityRequest unbuilt_request{
        unbuilt, pair_view.x, pair_view.y, pair_view.z, one_mass, one_smooth};

    std::vector<blitzar_core::Scalar> unbuilt_density{kSentinel, kSentinel};

    BLITZAR_CHECK(
        sph.Evaluate(unbuilt_request, unbuilt_density) == BLITZAR_STATUS_INVALID_ARGUMENT);

    return 0;
}

} // namespace

int main()
{
    BLITZAR_CHECK(CheckKernelNormalization() == 0);
    BLITZAR_CHECK(CheckTwoBodyExact() == 0);
    BLITZAR_CHECK(CheckUniformLatticeInterior() == 0);
    BLITZAR_CHECK(CheckShepardBoundary() == 0);
    BLITZAR_CHECK(CheckAnalyticField() == 0);
    BLITZAR_CHECK(CheckFailSafe() == 0);

    return 0;
}
