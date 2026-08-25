#include "MpiCases.hpp"
#include "Views.hpp"
#include "parallel/DomainDecomposition.hpp"
#include "particles/ParticleBuffer.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>

namespace blitzar_mpi_tests {

namespace {

bool CheckIncludedBoundaryPoints(const blitzar_parallel::DomainDecomposition& domain,
    std::span<const blitzar_core::Vector3> points, int rank_count,
    std::uint64_t& particle_id) noexcept
{
    for (const blitzar_core::Vector3 position : points) {
        const int owner = domain.Owner(position, particle_id++);

        if (!domain.Contains(position) || owner < 0 || owner >= rank_count) {
            return false;
        }
    }

    return true;
}

bool CheckExcludedBoundaryPoints(const blitzar_parallel::DomainDecomposition& domain,
    std::span<const blitzar_core::Vector3> points) noexcept
{
    for (const blitzar_core::Vector3 position : points) {
        if (domain.Contains(position) || domain.Owner(position) != -1) {
            return false;
        }
    }

    return true;
}

std::array<blitzar_core::Vector3, 8> MakeCorners(blitzar_parallel::DomainBounds bounds) noexcept
{
    std::array<blitzar_core::Vector3, 8> corners{};
    std::size_t corner_index = 0;

    for (const int x_side : {-1, 1}) {
        for (const int y_side : {-1, 1}) {
            for (const int z_side : {-1, 1}) {
                corners[corner_index++] = {x_side < 0 ? bounds.minimum.x : bounds.maximum.x,
                    y_side < 0 ? bounds.minimum.y : bounds.maximum.y,
                    z_side < 0 ? bounds.minimum.z : bounds.maximum.z};
            }
        }
    }

    return corners;
}

std::array<blitzar_core::Vector3, 6> MakeOutsideFaces(
    blitzar_parallel::DomainBounds bounds, blitzar_core::Vector3 middle) noexcept
{
    const double negative_infinity = -std::numeric_limits<double>::infinity();
    const double positive_infinity = std::numeric_limits<double>::infinity();

    return {blitzar_core::Vector3{
                std::nextafter(bounds.minimum.x, negative_infinity), middle.y, middle.z},
        blitzar_core::Vector3{
            std::nextafter(bounds.maximum.x, positive_infinity), middle.y, middle.z},
        blitzar_core::Vector3{
            middle.x, std::nextafter(bounds.minimum.y, negative_infinity), middle.z},
        blitzar_core::Vector3{
            middle.x, std::nextafter(bounds.maximum.y, positive_infinity), middle.z},
        blitzar_core::Vector3{
            middle.x, middle.y, std::nextafter(bounds.minimum.z, negative_infinity)},
        blitzar_core::Vector3{
            middle.x, middle.y, std::nextafter(bounds.maximum.z, positive_infinity)}};
}

std::array<blitzar_core::Vector3, 8> MakeOutsideCorners(
    blitzar_parallel::DomainBounds bounds) noexcept
{
    const double negative_infinity = -std::numeric_limits<double>::infinity();
    const double positive_infinity = std::numeric_limits<double>::infinity();
    std::array<blitzar_core::Vector3, 8> corners{};
    std::size_t corner_index = 0;

    for (const int x_side : {-1, 1}) {
        for (const int y_side : {-1, 1}) {
            for (const int z_side : {-1, 1}) {
                corners[corner_index++] = {
                    x_side < 0 ? std::nextafter(bounds.minimum.x, negative_infinity)
                               : std::nextafter(bounds.maximum.x, positive_infinity),
                    y_side < 0 ? std::nextafter(bounds.minimum.y, negative_infinity)
                               : std::nextafter(bounds.maximum.y, positive_infinity),
                    z_side < 0 ? std::nextafter(bounds.minimum.z, negative_infinity)
                               : std::nextafter(bounds.maximum.z, positive_infinity)};
            }
        }
    }

    return corners;
}

} // namespace

bool RunBoundaryOwnershipCase(blitzar_parallel::MpiContext& context) noexcept
{
    const StateArrays initial = InitialState();
    blitzar_particles::ParticleBuffer particles(ParticleCount);

    for (std::size_t index = 0; index < ParticleCount; ++index) {
        if (particles.SetPosition(index, {initial.x[index], initial.y[index], initial.z[index]}) !=

                BLITZAR_STATUS_OK ||
            particles.SetMass(index, initial.mass[index]) != BLITZAR_STATUS_OK) {
            return false;
        }
    }

    blitzar_parallel::DomainDecomposition domain;

    if (domain.Initialize(particles.State(), context) != BLITZAR_STATUS_OK) {
        return false;
    }

    const blitzar_parallel::DomainBounds bounds = domain.GlobalBounds();

    if (!bounds.IsValid()) {
        return false;
    }

    const blitzar_core::Vector3 middle{(bounds.minimum.x + bounds.maximum.x) * 0.5,
        (bounds.minimum.y + bounds.maximum.y) * 0.5, (bounds.minimum.z + bounds.maximum.z) * 0.5};

    const std::array<blitzar_core::Vector3, 6> faces{
        blitzar_core::Vector3{bounds.minimum.x, middle.y, middle.z},
        blitzar_core::Vector3{bounds.maximum.x, middle.y, middle.z},
        blitzar_core::Vector3{middle.x, bounds.minimum.y, middle.z},
        blitzar_core::Vector3{middle.x, bounds.maximum.y, middle.z},
        blitzar_core::Vector3{middle.x, middle.y, bounds.minimum.z},
        blitzar_core::Vector3{middle.x, middle.y, bounds.maximum.z}};

    std::uint64_t particle_id = 0;

    if (!CheckIncludedBoundaryPoints(domain, faces, context.Size(), particle_id)) {
        return false;
    }

    const std::array<blitzar_core::Vector3, 8> corners = MakeCorners(bounds);

    if (!CheckIncludedBoundaryPoints(domain, corners, context.Size(), particle_id)) {
        return false;
    }

    const std::array<blitzar_core::Vector3, 6> outside_faces = MakeOutsideFaces(bounds, middle);

    if (!CheckExcludedBoundaryPoints(domain, outside_faces)) {
        return false;
    }

    const std::array<blitzar_core::Vector3, 8> outside_corners = MakeOutsideCorners(bounds);

    return CheckExcludedBoundaryPoints(domain, outside_corners);
}

bool RunOutOfDomainCase() noexcept
{
    StateArrays initial = InitialState();

    initial.velocity_x.fill(1000.0);
    initial.velocity_y.fill(0.0);
    initial.velocity_z.fill(0.0);

    blitzar_sdk::Simulation simulation(ParticleCount);

    if (!Configure(simulation, initial, 1.0)) {
        return false;
    }

    StateArrays before{};

    if (simulation.GetState(blitzar_tests::MakeOutputView(before)) != BLITZAR_STATUS_OK ||
        simulation.Step() != BLITZAR_STATUS_INVALID_ARGUMENT) {
        return false;
    }

    StateArrays after{};

    if (simulation.GetState(blitzar_tests::MakeOutputView(after)) != BLITZAR_STATUS_OK) {
        return false;
    }

    for (std::size_t index = 0; index < ParticleCount; ++index) {
        if (after.x[index] != before.x[index] || after.y[index] != before.y[index] ||
            after.z[index] != before.z[index] ||
            after.velocity_x[index] != before.velocity_x[index] ||
            after.velocity_y[index] != before.velocity_y[index] ||
            after.velocity_z[index] != before.velocity_z[index] ||
            after.mass[index] != before.mass[index]) {
            return false;
        }
    }

    return true;
}

} // namespace blitzar_mpi_tests
