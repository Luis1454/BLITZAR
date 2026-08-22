#include "integration/LeapfrogKdk.hpp"
#include "parallel/DomainDecomposition.hpp"
#include "parallel/MpiContext.hpp"
#include "parallel/MpiExchange.hpp"
#include "particles/ParticleBuffer.hpp"
#include "sdk/Simulation.hpp"
#include "solvers/direct/DirectSolver.hpp"

#include "Check.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <span>

namespace {

constexpr std::size_t ParticleCount = 8;

using StateArray = std::array<double, ParticleCount>;

struct StateArrays final {
    StateArray x{};
    StateArray y{};
    StateArray z{};
    StateArray velocity_x{};
    StateArray velocity_y{};
    StateArray velocity_z{};
    StateArray mass{};
};

[[nodiscard]] StateArrays InitialState() noexcept
{
    StateArrays state{};
    for (std::size_t index = 0; index < ParticleCount; ++index) {
        const double value = static_cast<double>(index);
        state.x[index] = -3.5 + value;
        state.y[index] = (index % 2 == 0 ? -1.0 : 1.0) + 0.1 * value;
        state.z[index] = (index % 3 == 0 ? 1.0 : -1.0) - 0.05 * value;
        state.velocity_x[index] = 0.02 * (3.5 - value);
        state.velocity_y[index] = -0.01 * value;
        state.velocity_z[index] = 0.015 * value;
        state.mass[index] = 1.0 + 0.25 * static_cast<double>(index % 3);
    }
    return state;
}

[[nodiscard]] StateArrays MigrationState() noexcept
{
    StateArrays state = InitialState();
    for (std::size_t index = 0; index < ParticleCount; ++index) {
        state.velocity_x[index] = index % 2 == 0 ? 50.0 : -50.0;
        state.velocity_y[index] = 0.0;
        state.velocity_z[index] = 0.0;
    }
    return state;
}

[[nodiscard]] bool CheckIncludedBoundaryPoints(
    const blitzar_parallel::DomainDecomposition& domain,
    std::span<const blitzar_core::Vector3> points,
    int rank_count,
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

[[nodiscard]] bool CheckExcludedBoundaryPoints(
    const blitzar_parallel::DomainDecomposition& domain,
    std::span<const blitzar_core::Vector3> points) noexcept
{
    for (const blitzar_core::Vector3 position : points) {
        if (domain.Contains(position) || domain.Owner(position) != -1) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] std::array<blitzar_core::Vector3, 8> MakeCorners(
    blitzar_parallel::DomainBounds bounds) noexcept
{
    std::array<blitzar_core::Vector3, 8> corners{};
    std::size_t corner_index = 0;
    for (const int x_side : {-1, 1}) {
        for (const int y_side : {-1, 1}) {
            for (const int z_side : {-1, 1}) {
                corners[corner_index++] = {
                    x_side < 0 ? bounds.minimum.x : bounds.maximum.x,
                    y_side < 0 ? bounds.minimum.y : bounds.maximum.y,
                    z_side < 0 ? bounds.minimum.z : bounds.maximum.z};
            }
        }
    }
    return corners;
}

[[nodiscard]] std::array<blitzar_core::Vector3, 6> MakeOutsideFaces(
    blitzar_parallel::DomainBounds bounds,
    blitzar_core::Vector3 middle) noexcept
{
    const double negative_infinity = -std::numeric_limits<double>::infinity();
    const double positive_infinity = std::numeric_limits<double>::infinity();
    return {
        blitzar_core::Vector3{
            std::nextafter(bounds.minimum.x, negative_infinity),
            middle.y,
            middle.z},
        blitzar_core::Vector3{
            std::nextafter(bounds.maximum.x, positive_infinity),
            middle.y,
            middle.z},
        blitzar_core::Vector3{
            middle.x,
            std::nextafter(bounds.minimum.y, negative_infinity),
            middle.z},
        blitzar_core::Vector3{
            middle.x,
            std::nextafter(bounds.maximum.y, positive_infinity),
            middle.z},
        blitzar_core::Vector3{
            middle.x,
            middle.y,
            std::nextafter(bounds.minimum.z, negative_infinity)},
        blitzar_core::Vector3{
            middle.x,
            middle.y,
            std::nextafter(bounds.maximum.z, positive_infinity)}};
}

[[nodiscard]] std::array<blitzar_core::Vector3, 8> MakeOutsideCorners(
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
                    x_side < 0
                        ? std::nextafter(bounds.minimum.x, negative_infinity)
                        : std::nextafter(bounds.maximum.x, positive_infinity),
                    y_side < 0
                        ? std::nextafter(bounds.minimum.y, negative_infinity)
                        : std::nextafter(bounds.maximum.y, positive_infinity),
                    z_side < 0
                        ? std::nextafter(bounds.minimum.z, negative_infinity)
                        : std::nextafter(bounds.maximum.z, positive_infinity)};
            }
        }
    }
    return corners;
}

[[nodiscard]] bool RunBoundaryOwnershipCase(
    blitzar_parallel::MpiContext& context) noexcept
{
    const StateArrays initial = InitialState();
    blitzar_particles::ParticleBuffer particles(ParticleCount);
    for (std::size_t index = 0; index < ParticleCount; ++index) {
        if (particles.SetPosition(
                index, {initial.x[index], initial.y[index], initial.z[index]}) !=
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
    const blitzar_core::Vector3 middle{
        (bounds.minimum.x + bounds.maximum.x) * 0.5,
        (bounds.minimum.y + bounds.maximum.y) * 0.5,
        (bounds.minimum.z + bounds.maximum.z) * 0.5};

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
    if (!CheckIncludedBoundaryPoints(
            domain, corners, context.Size(), particle_id)) {
        return false;
    }

    const std::array<blitzar_core::Vector3, 6> outside_faces =
        MakeOutsideFaces(bounds, middle);
    if (!CheckExcludedBoundaryPoints(domain, outside_faces)) {
        return false;
    }

    const std::array<blitzar_core::Vector3, 8> outside_corners =
        MakeOutsideCorners(bounds);
    return CheckExcludedBoundaryPoints(domain, outside_corners);
}

[[nodiscard]] bool Configure(
    blitzar_sdk::Simulation& simulation,
    const StateArrays& state,
    double timestep) noexcept
{
    return simulation.SetSolver(BLITZAR_SOLVER_DIRECT) == BLITZAR_STATUS_OK &&
           simulation.SetGravity(1.0, 0.1) == BLITZAR_STATUS_OK &&
           simulation.SetTimestep(timestep) == BLITZAR_STATUS_OK &&
           simulation.SetParticles(
               state.x,
               state.y,
               state.z,
               state.velocity_x,
               state.velocity_y,
               state.velocity_z,
               state.mass) == BLITZAR_STATUS_OK;
}

[[nodiscard]] bool BuildReference(
    const StateArrays& initial,
    StateArrays& result,
    double timestep,
    int step_count) noexcept
{
    blitzar_particles::ParticleBuffer particles(ParticleCount);
    blitzar_particles::AccelerationBuffer accelerations(ParticleCount);
    blitzar_integration::LeapfrogWorkspace workspace(ParticleCount);
    for (std::size_t index = 0; index < ParticleCount; ++index) {
        if (particles.SetPosition(
                index,
                {initial.x[index], initial.y[index], initial.z[index]}) !=
                BLITZAR_STATUS_OK ||
            particles.SetVelocity(
                index,
                {initial.velocity_x[index],
                 initial.velocity_y[index],
                 initial.velocity_z[index]}) != BLITZAR_STATUS_OK ||
            particles.SetMass(index, initial.mass[index]) != BLITZAR_STATUS_OK) {
            return false;
        }
    }
    const blitzar_physics::GravityParameters gravity{1.0, 0.1};
    blitzar_direct::DirectSolver solver(gravity);
    const blitzar_core::ExecutionSettings execution{};
    const blitzar_integration::LeapfrogKdk integrator{};
    for (int step = 0; step < step_count; ++step) {
        if (integrator.Advance(
                particles,
                accelerations,
                workspace,
                solver,
                timestep,
                execution) != BLITZAR_STATUS_OK) {
            return false;
        }
    }
    const blitzar_core::ParticleStateView state = particles.State();
    for (std::size_t index = 0; index < ParticleCount; ++index) {
        result.x[index] = state.x[index];
        result.y[index] = state.y[index];
        result.z[index] = state.z[index];
        result.velocity_x[index] = state.velocity_x[index];
        result.velocity_y[index] = state.velocity_y[index];
        result.velocity_z[index] = state.velocity_z[index];
        result.mass[index] = state.mass[index];
    }
    return true;
}

[[nodiscard]] bool RunCase(
    const StateArrays& initial,
    double timestep,
    int step_count) noexcept
{
    StateArrays reference{};
    bool local_ok = BuildReference(
        initial,
        reference,
        timestep,
        step_count);

    blitzar_sdk::Simulation simulation(ParticleCount);
    const bool configuration_ok = Configure(simulation, initial, timestep);
    local_ok = local_ok && configuration_ok;
    for (int step = 0; step < step_count; ++step) {
        const blitzar_status step_status = simulation.Step();
        local_ok = local_ok && step_status == BLITZAR_STATUS_OK;
    }

    StateArrays distributed{};
    const blitzar_status state_status = simulation.GetState(
        distributed.x,
        distributed.y,
        distributed.z,
        distributed.velocity_x,
        distributed.velocity_y,
        distributed.velocity_z,
        distributed.mass);
    local_ok = local_ok && state_status == BLITZAR_STATUS_OK;
    for (std::size_t index = 0; index < ParticleCount; ++index) {
        local_ok = local_ok &&
                   std::abs(distributed.x[index] - reference.x[index]) < 1.0e-5 &&
                   std::abs(distributed.y[index] - reference.y[index]) < 1.0e-5 &&
                   std::abs(distributed.z[index] - reference.z[index]) < 1.0e-5 &&
                   std::abs(distributed.velocity_x[index] -
                            reference.velocity_x[index]) < 1.0e-5 &&
                   std::abs(distributed.velocity_y[index] -
                            reference.velocity_y[index]) < 1.0e-5 &&
                   std::abs(distributed.velocity_z[index] -
                            reference.velocity_z[index]) < 1.0e-5 &&
                   distributed.mass[index] == reference.mass[index];
    }

    StateArrays rejected = initial;
    rejected.x[0] += 100.0;
    rejected.mass[0] = -1.0;
    local_ok = local_ok &&
               simulation.SetParticles(
                   rejected.x,
                   rejected.y,
                   rejected.z,
                   rejected.velocity_x,
                   rejected.velocity_y,
                   rejected.velocity_z,
                   rejected.mass) == BLITZAR_STATUS_INVALID_ARGUMENT;

    StateArrays after_rejected{};
    const blitzar_status rejected_state_status = simulation.GetState(
        after_rejected.x,
        after_rejected.y,
        after_rejected.z,
        after_rejected.velocity_x,
        after_rejected.velocity_y,
        after_rejected.velocity_z,
        after_rejected.mass);
    local_ok = local_ok && rejected_state_status == BLITZAR_STATUS_OK;
    for (std::size_t index = 0; index < ParticleCount; ++index) {
        local_ok = local_ok &&
                   std::abs(after_rejected.x[index] - reference.x[index]) <
                       1.0e-5 &&
                   std::abs(after_rejected.y[index] - reference.y[index]) <
                       1.0e-5 &&
                   std::abs(after_rejected.z[index] - reference.z[index]) <
                       1.0e-5 &&
                   std::abs(after_rejected.velocity_x[index] -
                            reference.velocity_x[index]) <
                       1.0e-5 &&
                   std::abs(after_rejected.velocity_y[index] -
                            reference.velocity_y[index]) <
                       1.0e-5 &&
                   std::abs(after_rejected.velocity_z[index] -
                            reference.velocity_z[index]) <
                       1.0e-5 &&
                   after_rejected.mass[index] == reference.mass[index];
    }
    return local_ok;
}

[[nodiscard]] bool RunRollbackCase() noexcept
{
    StateArrays initial{};
    initial.x = {0.0, 1.0, 10.0, 11.0, 20.0, 21.0, 30.0, 31.0};
    initial.velocity_x.fill(0.0);
    for (std::size_t index = 1; index < ParticleCount; index += 2) {
        initial.velocity_x[index] = -1.0;
    }
    initial.velocity_y.fill(0.0);
    initial.velocity_z.fill(0.0);
    initial.mass.fill(1.0);

    blitzar_sdk::Simulation simulation(ParticleCount);
    blitzar_sdk::Simulation expected(ParticleCount);
    const auto configure = [&initial](blitzar_sdk::Simulation& candidate) {
        return Configure(candidate, initial, 0.5) &&
               candidate.SetGravity(
                   std::numeric_limits<double>::denorm_min(),
                   0.1) == BLITZAR_STATUS_OK;
    };
    if (!configure(simulation) || !configure(expected) ||
        simulation.Step() != BLITZAR_STATUS_OK ||
        expected.Step() != BLITZAR_STATUS_OK) {
        return false;
    }

    StateArrays before_failure{};
    if (simulation.GetState(
            before_failure.x,
            before_failure.y,
            before_failure.z,
            before_failure.velocity_x,
            before_failure.velocity_y,
            before_failure.velocity_z,
            before_failure.mass) != BLITZAR_STATUS_OK ||
        simulation.SetGravity(
            std::numeric_limits<double>::denorm_min(),
            0.0) != BLITZAR_STATUS_OK ||
        simulation.Step() != BLITZAR_STATUS_SINGULARITY) {
        return false;
    }

    StateArrays restored{};
    if (simulation.GetState(
            restored.x,
            restored.y,
            restored.z,
            restored.velocity_x,
            restored.velocity_y,
            restored.velocity_z,
            restored.mass) != BLITZAR_STATUS_OK) {
        return false;
    }
    for (std::size_t index = 0; index < ParticleCount; ++index) {
        if (std::abs(restored.x[index] - before_failure.x[index]) > 1.0e-12 ||
            std::abs(restored.y[index] - before_failure.y[index]) > 1.0e-12 ||
            std::abs(restored.z[index] - before_failure.z[index]) > 1.0e-12 ||
            std::abs(restored.velocity_x[index] -
                     before_failure.velocity_x[index]) > 1.0e-12 ||
            std::abs(restored.velocity_y[index] -
                     before_failure.velocity_y[index]) > 1.0e-12 ||
            std::abs(restored.velocity_z[index] -
                     before_failure.velocity_z[index]) > 1.0e-12 ||
            restored.mass[index] != before_failure.mass[index]) {
            return false;
        }
    }

    if (simulation.SetGravity(
            std::numeric_limits<double>::denorm_min(),
            0.1) != BLITZAR_STATUS_OK ||
        expected.SetGravity(
            std::numeric_limits<double>::denorm_min(),
            0.1) != BLITZAR_STATUS_OK ||
        simulation.Step() != BLITZAR_STATUS_OK ||
        expected.Step() != BLITZAR_STATUS_OK) {
        return false;
    }

    StateArrays actual_retry{};
    StateArrays expected_retry{};
    if (simulation.GetState(
            actual_retry.x,
            actual_retry.y,
            actual_retry.z,
            actual_retry.velocity_x,
            actual_retry.velocity_y,
            actual_retry.velocity_z,
            actual_retry.mass) != BLITZAR_STATUS_OK ||
        expected.GetState(
            expected_retry.x,
            expected_retry.y,
            expected_retry.z,
            expected_retry.velocity_x,
            expected_retry.velocity_y,
            expected_retry.velocity_z,
            expected_retry.mass) != BLITZAR_STATUS_OK) {
        return false;
    }
    for (std::size_t index = 0; index < ParticleCount; ++index) {
        if (std::abs(actual_retry.x[index] - expected_retry.x[index]) >
                1.0e-12 ||
            std::abs(actual_retry.y[index] - expected_retry.y[index]) >
                1.0e-12 ||
            std::abs(actual_retry.z[index] - expected_retry.z[index]) >
                1.0e-12 ||
            std::abs(actual_retry.velocity_x[index] -
                     expected_retry.velocity_x[index]) > 1.0e-12 ||
            std::abs(actual_retry.velocity_y[index] -
                     expected_retry.velocity_y[index]) > 1.0e-12 ||
            std::abs(actual_retry.velocity_z[index] -
                     expected_retry.velocity_z[index]) > 1.0e-12 ||
            actual_retry.mass[index] != expected_retry.mass[index]) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool RunErrorSynchronizationCase(
    blitzar_parallel::MpiContext& context) noexcept
{
    blitzar_status global_status = BLITZAR_STATUS_OK;
    const blitzar_status local_status = context.Rank() == 0
                                            ? BLITZAR_STATUS_INTERNAL_ERROR
                                            : BLITZAR_STATUS_OK;
    if (context.SynchronizeStatus(
            local_status,
            "MpiTest",
            "injected-failure",
            global_status) != BLITZAR_STATUS_OK ||
        global_status != BLITZAR_STATUS_INTERNAL_ERROR) {
        return false;
    }

    blitzar_particles::ParticleBuffer particles(1);
    if (particles.SetPosition(0, {0.0, 0.0, 0.0}) != BLITZAR_STATUS_OK ||
        particles.SetMass(0, 1.0) != BLITZAR_STATUS_OK) {
        return false;
    }
    blitzar_parallel::DomainDecomposition domain;
    if (domain.Initialize(particles.State(), context) != BLITZAR_STATUS_OK) {
        return false;
    }
    blitzar_parallel::MpiExchange exchange(context, domain);
    const std::array<std::uint64_t, 1> ids{0};

    blitzar_parallel::MpiContext::GhostExchange pre_completion_exchange;
    if (exchange.BeginGhosts(
            particles.State(), ids, pre_completion_exchange) !=
        BLITZAR_STATUS_OK) {
        return false;
    }
    if (context.Rank() == 0) {
        context.AbortGhostExchange(pre_completion_exchange);
    }
    blitzar_parallel::PacketBuffer aborted_ghosts;
    aborted_ghosts.Resize(1);
    if (exchange.CompleteGhosts(
            pre_completion_exchange, aborted_ghosts) !=
            BLITZAR_STATUS_INVALID_ARGUMENT ||
        aborted_ghosts.Size() != 0) {
        return false;
    }
    blitzar_parallel::PacketBuffer recovered_ghosts;
    if (exchange.ExchangeGhosts(particles.State(), ids, recovered_ghosts) !=
        BLITZAR_STATUS_OK) {
        return false;
    }

    blitzar_core::ParticleStateView invalid_state{};
    invalid_state.count = 1;
    invalid_state.source_count = 1;
    const blitzar_core::ParticleStateView local_state =
        context.Rank() == 0 ? invalid_state
                             : particles.State();
    const std::span<const std::uint64_t> local_ids =
        context.Rank() == 0 ? std::span<const std::uint64_t>{}
                             : std::span<const std::uint64_t>(ids);

    blitzar_parallel::MpiContext::GhostExchange ghost_exchange;
    if (exchange.BeginGhosts(local_state, local_ids, ghost_exchange) !=
            BLITZAR_STATUS_INVALID_ARGUMENT ||
        context.IsGhostExchangeActive(ghost_exchange)) {
        return false;
    }

    blitzar_parallel::PacketBuffer ghosts;
    if (exchange.CompleteGhosts(ghost_exchange, ghosts) !=
            BLITZAR_STATUS_INVALID_ARGUMENT ||
        ghosts.Size() != 0) {
        return false;
    }

    blitzar_parallel::PacketBuffer received;
    if (exchange.Migrate(local_state, local_ids, received) !=
            BLITZAR_STATUS_INVALID_ARGUMENT ||
        received.Size() != 0) {
        return false;
    }

    blitzar_particles::ParticleBuffer escaped(1);
    const blitzar_parallel::DomainBounds bounds = domain.GlobalBounds();
    const double escaped_x = context.Rank() == 0
                                 ? std::nextafter(
                                       bounds.maximum.x,
                                       std::numeric_limits<double>::infinity())
                                 : bounds.maximum.x;
    if (escaped.SetPosition(0, {escaped_x, 0.0, 0.0}) !=
            BLITZAR_STATUS_OK ||
        exchange.Migrate(escaped.State(), ids, received) !=
            BLITZAR_STATUS_INVALID_ARGUMENT ||
        received.Size() != 0) {
        return false;
    }

    blitzar_parallel::DomainDecomposition uninitialized_domain;
    blitzar_parallel::MpiExchange uninitialized_exchange(
        context, uninitialized_domain);
    blitzar_parallel::PacketBuffer uninitialized_received;
    return uninitialized_exchange.Migrate(
               particles.State(), ids, uninitialized_received) ==
               BLITZAR_STATUS_INVALID_ARGUMENT &&
           uninitialized_received.Size() == 0;
}

}  // namespace

int main(int argc, char** argv)
{
    (void)argv;
    blitzar_parallel::MpiContext context;
    const bool valid_world =
        context.IsUsable() && (context.Size() == 2 || context.Size() == 4);
    const bool migration_case = argc > 1;
    const bool local_case = RunCase(
        migration_case ? MigrationState() : InitialState(),
        0.01,
        migration_case ? 1 : 2);
    const bool rollback_case = RunRollbackCase();
    const bool boundary_case = RunBoundaryOwnershipCase(context);
    const bool error_synchronization_case = RunErrorSynchronizationCase(context);
    const bool local_ok =
        valid_world && local_case && rollback_case && boundary_case &&
        error_synchronization_case;

    int local_failure = local_ok ? 0 : 1;
    int global_failure = 0;
    BLITZAR_CHECK(
        context.ReduceMax(local_failure, global_failure) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(global_failure == 0);
    return 0;
}
