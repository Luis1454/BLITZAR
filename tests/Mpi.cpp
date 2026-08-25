#include "AllocationMonitor.hpp"
#include "Check.hpp"
#include "Views.hpp"
#include "integration/LeapfrogKdk.hpp"
#include "parallel/DomainDecomposition.hpp"
#include "parallel/MpiContext.hpp"
#include "parallel/MpiExchange.hpp"
#include "parallel/MpiTypes.hpp"
#include "particles/AccelerationBuffer.hpp"
#include "particles/ParticleBuffer.hpp"
#include "sdk/Simulation.hpp"
#include "solvers/direct/DirectSolver.hpp"

#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <limits>
#include <span>
#include <string_view>
#include <utility>

#if defined(BLITZAR_HAS_MPI)
#include <mpi.h>
#endif

#if defined(__linux__)
#include <sys/resource.h>
#endif

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

[[nodiscard]] bool CheckIncludedBoundaryPoints(const blitzar_parallel::DomainDecomposition& domain,
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

[[nodiscard]] bool CheckExcludedBoundaryPoints(const blitzar_parallel::DomainDecomposition& domain,
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
                corners[corner_index++] = {x_side < 0 ? bounds.minimum.x : bounds.maximum.x,
                    y_side < 0 ? bounds.minimum.y : bounds.maximum.y,
                    z_side < 0 ? bounds.minimum.z : bounds.maximum.z};
            }
        }
    }

    return corners;
}

[[nodiscard]] std::array<blitzar_core::Vector3, 6> MakeOutsideFaces(
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

[[nodiscard]] bool RunBoundaryOwnershipCase(blitzar_parallel::MpiContext& context) noexcept
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

[[nodiscard]] bool Configure(blitzar_sdk::Simulation& simulation, const StateArrays& state,
    double timestep, blitzar_solver_kind solver_kind = BLITZAR_SOLVER_DIRECT) noexcept
{
    if (solver_kind == BLITZAR_SOLVER_BARNES_HUT &&
        simulation.SetBarnesHut({0.0, ParticleCount, 128, 1, 32}) != BLITZAR_STATUS_OK) {
        return false;
    }

    blitzar_core::ParticleStateView input = blitzar_tests::MakeStateView(state);

#if defined(BLITZAR_HAS_MPI)
    int rank = 0;

    if (MPI_Comm_rank(MPI_COMM_WORLD, &rank) != MPI_SUCCESS) {
        return false;
    }

    if (rank != 0) {
        input = {};
    }

#endif

    return simulation.SetSolver(solver_kind) == BLITZAR_STATUS_OK &&
           simulation.SetGravity(1.0, 0.1) == BLITZAR_STATUS_OK &&
           simulation.SetTimestep(timestep) == BLITZAR_STATUS_OK &&
           simulation.SetParticles(input) == BLITZAR_STATUS_OK;
}

[[nodiscard]] bool BuildReference(
    const StateArrays& initial, StateArrays& result, double timestep, int step_count) noexcept
{
    blitzar_particles::ParticleBuffer particles(ParticleCount);
    blitzar_particles::AccelerationBuffer accelerations(ParticleCount);
    blitzar_integration::KdkCheckpoint checkpoint(ParticleCount);

    for (std::size_t index = 0; index < ParticleCount; ++index) {
        if (particles.SetPosition(index, {initial.x[index], initial.y[index], initial.z[index]}) !=

                BLITZAR_STATUS_OK ||
            particles.SetVelocity(index, {initial.velocity_x[index], initial.velocity_y[index],
                                             initial.velocity_z[index]}) != BLITZAR_STATUS_OK ||
            particles.SetMass(index, initial.mass[index]) != BLITZAR_STATUS_OK) {
            return false;
        }
    }

    const blitzar_physics::GravityParameters gravity{1.0, 0.1};
    blitzar_direct::DirectSolver solver(gravity);

    if (solver.Prepare(ParticleCount) != BLITZAR_STATUS_OK) {
        return false;
    }

    const blitzar_core::ExecutionSettings execution{};
    const blitzar_integration::LeapfrogKdk integrator{};
    std::span<std::size_t> solver_scratch{};

    for (int step = 0; step < step_count; ++step) {
        blitzar_integration_kdk::AdvanceState state{particles, accelerations, checkpoint, solver,
            timestep, execution, solver_scratch, particles.State()};

        if (integrator.Advance(state) != BLITZAR_STATUS_OK) {
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

[[nodiscard]] bool RunCase(const StateArrays& initial, double timestep, int step_count,
    blitzar_solver_kind solver_kind = BLITZAR_SOLVER_DIRECT) noexcept
{
    StateArrays reference{};
    bool local_ok = BuildReference(initial, reference, timestep, step_count);
    blitzar_sdk::Simulation simulation(ParticleCount);
    const bool configuration_ok = Configure(simulation, initial, timestep, solver_kind);

    local_ok = local_ok && configuration_ok;

    for (int step = 0; step < step_count; ++step) {
        const blitzar_status step_status = simulation.Step();

        local_ok = local_ok && step_status == BLITZAR_STATUS_OK;
    }

    StateArrays distributed{};
    const blitzar_status state_status =
        simulation.GetState(blitzar_tests::MakeOutputView(distributed));

    local_ok = local_ok && state_status == BLITZAR_STATUS_OK;

    for (std::size_t index = 0; index < ParticleCount; ++index) {
        const bool state_matches =
            std::abs(distributed.x[index] - reference.x[index]) < 1.0e-5 &&
            std::abs(distributed.y[index] - reference.y[index]) < 1.0e-5 &&
            std::abs(distributed.z[index] - reference.z[index]) < 1.0e-5 &&
            std::abs(distributed.velocity_x[index] - reference.velocity_x[index]) < 1.0e-5 &&
            std::abs(distributed.velocity_y[index] - reference.velocity_y[index]) < 1.0e-5 &&
            std::abs(distributed.velocity_z[index] - reference.velocity_z[index]) < 1.0e-5 &&
            distributed.mass[index] == reference.mass[index];

        local_ok = local_ok && state_matches;
    }

    StateArrays rejected = initial;

    rejected.x[0] += 100.0;
    rejected.mass[0] = -1.0;
    local_ok = local_ok && simulation.SetParticles(blitzar_tests::MakeStateView(rejected)) ==
                               BLITZAR_STATUS_INVALID_ARGUMENT;

    StateArrays after_rejected{};
    const blitzar_status rejected_state_status =
        simulation.GetState(blitzar_tests::MakeOutputView(after_rejected));

    local_ok = local_ok && rejected_state_status == BLITZAR_STATUS_OK;

    for (std::size_t index = 0; index < ParticleCount; ++index) {
        local_ok =
            local_ok && std::abs(after_rejected.x[index] - reference.x[index]) < 1.0e-5 &&
            std::abs(after_rejected.y[index] - reference.y[index]) < 1.0e-5 &&
            std::abs(after_rejected.z[index] - reference.z[index]) < 1.0e-5 &&
            std::abs(after_rejected.velocity_x[index] - reference.velocity_x[index]) < 1.0e-5 &&
            std::abs(after_rejected.velocity_y[index] - reference.velocity_y[index]) < 1.0e-5 &&
            std::abs(after_rejected.velocity_z[index] - reference.velocity_z[index]) < 1.0e-5 &&
            after_rejected.mass[index] == reference.mass[index];
    }

    return local_ok;
}

[[nodiscard]] bool StatesMatch(const StateArrays& left, const StateArrays& right) noexcept
{
    for (std::size_t index = 0; index < ParticleCount; ++index) {
        if (std::abs(left.x[index] - right.x[index]) >= 1.0e-10 ||
            std::abs(left.y[index] - right.y[index]) >= 1.0e-10 ||
            std::abs(left.z[index] - right.z[index]) >= 1.0e-10 ||
            std::abs(left.velocity_x[index] - right.velocity_x[index]) >= 1.0e-10 ||
            std::abs(left.velocity_y[index] - right.velocity_y[index]) >= 1.0e-10 ||
            std::abs(left.velocity_z[index] - right.velocity_z[index]) >= 1.0e-10 ||
            left.mass[index] != right.mass[index]) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] bool RunOverlapCase(blitzar_parallel::MpiContext& context) noexcept
{
    const StateArrays initial = InitialState();
    blitzar_sdk::Simulation overlapped(ParticleCount);
    blitzar_sdk::Simulation serialized(ParticleCount);

    if (!Configure(overlapped, initial, 0.01) || !Configure(serialized, initial, 0.01)) {
        return false;
    }

    overlapped.SetMpiOverlapForTesting(blitzar_parallel::MpiOverlapMode::Overlapped);
    serialized.SetMpiOverlapForTesting(blitzar_parallel::MpiOverlapMode::Serialized);

    if (overlapped.Step() != BLITZAR_STATUS_OK || serialized.Step() != BLITZAR_STATUS_OK) {
        return false;
    }

    StateArrays overlapped_state{};
    StateArrays serialized_state{};

    if (overlapped.GetState(blitzar_tests::MakeOutputView(overlapped_state)) != BLITZAR_STATUS_OK ||
        serialized.GetState(blitzar_tests::MakeOutputView(serialized_state)) != BLITZAR_STATUS_OK) {
        return false;
    }

    const blitzar_parallel::MpiOverlapTrace& overlap_trace = overlapped.LastMpiOverlapTrace();

    const blitzar_parallel::MpiOverlapTrace& serialized_trace = serialized.LastMpiOverlapTrace();

    const bool parity = StatesMatch(overlapped_state, serialized_state);
    const bool volume_match = overlap_trace.local_packets == serialized_trace.local_packets &&
                              overlap_trace.ghost_packets == serialized_trace.ghost_packets &&
                              overlap_trace.send_bytes == serialized_trace.send_bytes &&
                              overlap_trace.receive_bytes == serialized_trace.receive_bytes;

    const bool timeline_valid = overlap_trace.status == BLITZAR_STATUS_OK &&
                                serialized_trace.status == BLITZAR_STATUS_OK &&
                                overlap_trace.HasOverlap() && !serialized_trace.HasOverlap() &&
                                overlap_trace.total_ns > 0 && serialized_trace.total_ns > 0;

    const double speedup = static_cast<double>(serialized_trace.total_ns) /
                           static_cast<double>(overlap_trace.total_ns);

    if (context.Rank() == 0) {
        std::fprintf(stdout,
            "BLITZAR MPI overlap ranks=%d local_packets=%zu ghost_packets=%zu "
            "send_bytes=%zu receive_bytes=%zu serialized_ns=%llu overlapped_ns=%llu "
            "speedup=%.6f parity=%d\n",
            context.Size(), overlap_trace.local_packets, overlap_trace.ghost_packets,
            overlap_trace.send_bytes, overlap_trace.receive_bytes,
            static_cast<unsigned long long>(serialized_trace.total_ns),
            static_cast<unsigned long long>(overlap_trace.total_ns), speedup, parity ? 1 : 0);
    }

    return parity && volume_match && timeline_valid;
}

[[nodiscard]] bool RunAllocationCase() noexcept
{
    const StateArrays initial = InitialState();
    const std::array<blitzar_solver_kind, 2> solvers{
        BLITZAR_SOLVER_DIRECT, BLITZAR_SOLVER_BARNES_HUT};

    for (const blitzar_solver_kind solver_kind : solvers) {
        blitzar_sdk::Simulation simulation(ParticleCount);
        StateArrays output{};

        const bool configured = Configure(simulation, initial, 0.01, solver_kind);

        if (!configured) {
            return false;
        }

        const blitzar_status warmup_status = simulation.Step();
        StateArrays warmup{};

        if (warmup_status != BLITZAR_STATUS_OK ||
            simulation.GetState(blitzar_tests::MakeOutputView(warmup)) != BLITZAR_STATUS_OK) {
            return false;
        }

        blitzar_tests::BeginAllocationCounting();

        const blitzar_status first_step = simulation.Step();
        const blitzar_status state_status =
            simulation.GetState(blitzar_tests::MakeOutputView(output));

        const std::size_t allocations = blitzar_tests::EndAllocationCounting();

        if (first_step != BLITZAR_STATUS_OK || state_status != BLITZAR_STATUS_OK ||
            allocations != 0) {
            return false;
        }
    }

    return true;
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
               candidate.SetGravity(std::numeric_limits<double>::denorm_min(), 0.1) ==
                   BLITZAR_STATUS_OK;
    };

    if (!configure(simulation) || !configure(expected) || simulation.Step() != BLITZAR_STATUS_OK ||
        expected.Step() != BLITZAR_STATUS_OK) {
        return false;
    }

    StateArrays before_failure{};

    if (simulation.GetState(blitzar_tests::MakeOutputView(before_failure)) != BLITZAR_STATUS_OK ||
        simulation.SetGravity(std::numeric_limits<double>::denorm_min(), 0.0) !=
            BLITZAR_STATUS_OK ||
        simulation.Step() != BLITZAR_STATUS_SINGULARITY) {
        return false;
    }

    StateArrays restored{};

    if (simulation.GetState(blitzar_tests::MakeOutputView(restored)) != BLITZAR_STATUS_OK) {
        return false;
    }

    for (std::size_t index = 0; index < ParticleCount; ++index) {
        if (std::abs(restored.x[index] - before_failure.x[index]) > 1.0e-12 ||
            std::abs(restored.y[index] - before_failure.y[index]) > 1.0e-12 ||
            std::abs(restored.z[index] - before_failure.z[index]) > 1.0e-12 ||
            std::abs(restored.velocity_x[index] - before_failure.velocity_x[index]) > 1.0e-12 ||
            std::abs(restored.velocity_y[index] - before_failure.velocity_y[index]) > 1.0e-12 ||
            std::abs(restored.velocity_z[index] - before_failure.velocity_z[index]) > 1.0e-12 ||
            restored.mass[index] != before_failure.mass[index]) {
            return false;
        }
    }

    if (simulation.SetGravity(std::numeric_limits<double>::denorm_min(), 0.1) !=
            BLITZAR_STATUS_OK ||
        expected.SetGravity(std::numeric_limits<double>::denorm_min(), 0.1) != BLITZAR_STATUS_OK ||
        simulation.Step() != BLITZAR_STATUS_OK || expected.Step() != BLITZAR_STATUS_OK) {
        return false;
    }

    StateArrays actual_retry{};
    StateArrays expected_retry{};

    if (simulation.GetState(blitzar_tests::MakeOutputView(actual_retry)) != BLITZAR_STATUS_OK ||
        expected.GetState(blitzar_tests::MakeOutputView(expected_retry)) != BLITZAR_STATUS_OK) {
        return false;
    }

    for (std::size_t index = 0; index < ParticleCount; ++index) {
        if (std::abs(actual_retry.x[index] - expected_retry.x[index]) > 1.0e-12 ||
            std::abs(actual_retry.y[index] - expected_retry.y[index]) > 1.0e-12 ||
            std::abs(actual_retry.z[index] - expected_retry.z[index]) > 1.0e-12 ||
            std::abs(actual_retry.velocity_x[index] - expected_retry.velocity_x[index]) > 1.0e-12 ||
            std::abs(actual_retry.velocity_y[index] - expected_retry.velocity_y[index]) > 1.0e-12 ||
            std::abs(actual_retry.velocity_z[index] - expected_retry.velocity_z[index]) > 1.0e-12 ||
            actual_retry.mass[index] != expected_retry.mass[index]) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] bool RunOutOfDomainCase() noexcept
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

[[nodiscard]] bool RunErrorSynchronizationCase(blitzar_parallel::MpiContext& context) noexcept
{
    blitzar_status global_status = BLITZAR_STATUS_OK;
    const blitzar_status local_status =
        context.Rank() == 0 ? BLITZAR_STATUS_INTERNAL_ERROR : BLITZAR_STATUS_OK;

    if (context.SynchronizeStatus(local_status, "MpiTest", "injected-failure", global_status) !=
            BLITZAR_STATUS_OK ||
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

    const std::size_t packet_capacity = static_cast<std::size_t>(context.Size());
    blitzar_parallel::MpiExchange exchange(context, domain, packet_capacity);
    const std::array<std::uint64_t, 1> ids{0};
    blitzar_parallel::MpiContext::GhostExchange unprepared_exchange;
    const blitzar_status expected_unprepared_begin =
        context.IsDistributed() ? BLITZAR_STATUS_INVALID_ARGUMENT : BLITZAR_STATUS_OK;

    if (exchange.BeginGhosts(particles.State(), ids, unprepared_exchange) !=
            expected_unprepared_begin ||
        context.IsGhostExchangeActive(unprepared_exchange)) {
        return false;
    }

    blitzar_parallel::MpiContext::GhostExchange pre_completion_exchange;

    if (context.PrepareCapacity(packet_capacity, pre_completion_exchange) != BLITZAR_STATUS_OK) {
        return false;
    }
    if (exchange.BeginGhosts(particles.State(), ids, pre_completion_exchange) !=
        BLITZAR_STATUS_OK) {
        return false;
    }
    if (context.IsDistributed() && !context.IsGhostExchangeActive(pre_completion_exchange)) {
        return false;
    }

    if (context.IsDistributed() && context.Rank() == 0) {
        context.AbortGhostExchange(pre_completion_exchange);
    }

    blitzar_parallel::PacketBuffer aborted_ghosts;

    aborted_ghosts.Reserve(1);
    aborted_ghosts.Resize(1);

    const blitzar_status aborted_completion_status =
        exchange.CompleteGhosts(pre_completion_exchange, aborted_ghosts);

    const blitzar_status expected_aborted_completion =
        context.IsDistributed() ? BLITZAR_STATUS_INVALID_ARGUMENT : BLITZAR_STATUS_OK;

    if (aborted_completion_status != expected_aborted_completion || aborted_ghosts.Size() != 0) {
        return false;
    }

    blitzar_parallel::PacketBuffer recovered_ghosts;

    recovered_ghosts.Reserve(static_cast<std::size_t>(context.Size()));

    if (exchange.ExchangeGhosts(particles.State(), ids, recovered_ghosts) != BLITZAR_STATUS_OK ||
        recovered_ghosts.Size() != static_cast<std::size_t>(context.Size() - 1)) {
        return false;
    }

    blitzar_parallel::MpiContext::GhostExchange active_exchange;
    blitzar_parallel::MpiContext::GhostExchange replacement_exchange;

    if (context.PrepareCapacity(packet_capacity, active_exchange) != BLITZAR_STATUS_OK ||
        context.PrepareCapacity(packet_capacity, replacement_exchange) != BLITZAR_STATUS_OK ||
        exchange.BeginGhosts(particles.State(), ids, active_exchange) != BLITZAR_STATUS_OK) {
        return false;
    }

    active_exchange = std::move(replacement_exchange);

    if (context.IsGhostExchangeActive(active_exchange) ||
        context.IsGhostExchangeActive(replacement_exchange)) {
        return false;
    }

    blitzar_parallel::PacketBuffer moved_ghosts;
    const blitzar_status moved_completion_status =
        exchange.CompleteGhosts(active_exchange, moved_ghosts);

    const blitzar_status expected_moved_completion =
        context.IsDistributed() ? BLITZAR_STATUS_INVALID_ARGUMENT : BLITZAR_STATUS_OK;

    if (moved_completion_status != expected_moved_completion || moved_ghosts.Size() != 0) {
        return false;
    }

    blitzar_parallel::PacketBuffer recovered_after_move;

    recovered_after_move.Reserve(static_cast<std::size_t>(context.Size()));

    if (exchange.ExchangeGhosts(particles.State(), ids, recovered_after_move) !=
            BLITZAR_STATUS_OK ||
        recovered_after_move.Size() != static_cast<std::size_t>(context.Size() - 1)) {
        return false;
    }

    blitzar_core::ParticleStateView invalid_state{};

    invalid_state.count = 1;
    invalid_state.source_count = 1;

    const blitzar_core::ParticleStateView local_state =
        context.Rank() == 0 ? invalid_state : particles.State();

    const std::span<const std::uint64_t> local_ids = context.Rank() == 0
                                                         ? std::span<const std::uint64_t>{}
                                                         : std::span<const std::uint64_t>(ids);

    blitzar_parallel::MpiContext::GhostExchange ghost_exchange;

    if (context.PrepareCapacity(packet_capacity, ghost_exchange) != BLITZAR_STATUS_OK) {
        return false;
    }
    if (exchange.BeginGhosts(local_state, local_ids, ghost_exchange) !=
            BLITZAR_STATUS_INVALID_ARGUMENT ||
        context.IsGhostExchangeActive(ghost_exchange)) {
        return false;
    }

    blitzar_parallel::PacketBuffer ghosts;
    const blitzar_status invalid_ghost_completion_status =
        exchange.CompleteGhosts(ghost_exchange, ghosts);

    const blitzar_status expected_invalid_ghost_completion =
        context.IsDistributed() ? BLITZAR_STATUS_INVALID_ARGUMENT : BLITZAR_STATUS_OK;

    if (invalid_ghost_completion_status != expected_invalid_ghost_completion ||
        ghosts.Size() != 0) {
        return false;
    }

    blitzar_parallel::PacketBuffer received;

    received.Reserve(1);

    if (exchange.Migrate(local_state, local_ids, received) != BLITZAR_STATUS_INVALID_ARGUMENT ||
        received.Size() != 0) {
        return false;
    }

    blitzar_particles::ParticleBuffer escaped(1);
    const blitzar_parallel::DomainBounds bounds = domain.GlobalBounds();
    const double escaped_x = context.Rank() == 0 ? std::nextafter(bounds.maximum.x,
                                                       std::numeric_limits<double>::infinity())
                                                 : bounds.maximum.x;

    if (escaped.SetPosition(0, {escaped_x, 0.0, 0.0}) != BLITZAR_STATUS_OK ||

        exchange.Migrate(escaped.State(), ids, received) != BLITZAR_STATUS_INVALID_ARGUMENT ||
        received.Size() != 0) {
        return false;
    }

    blitzar_parallel::DomainDecomposition uninitialized_domain;

    blitzar_parallel::MpiExchange uninitialized_exchange(
        context, uninitialized_domain, packet_capacity);

    blitzar_parallel::PacketBuffer uninitialized_received;

    return uninitialized_exchange.Migrate(particles.State(), ids, uninitialized_received) ==
               BLITZAR_STATUS_INVALID_ARGUMENT &&
           uninitialized_received.Size() == 0;
}

[[nodiscard]] bool RunNestedContextCase(const blitzar_parallel::MpiContext& outer) noexcept
{
    blitzar_parallel::MpiContext nested;

    return nested.IsUsable() && nested.Rank() == outer.Rank() && nested.Size() == outer.Size();
}

[[nodiscard]] bool RunCollectiveValidationCase(const blitzar_parallel::MpiContext& context) noexcept
{
    const std::array<int, 1> invalid_counts{0};
    std::array<int, 1> invalid_receive{};
    const blitzar_status expected_zero_layout =
        context.IsDistributed() ? BLITZAR_STATUS_INVALID_ARGUMENT : BLITZAR_STATUS_OK;

    if (context.AllToAllCounts(invalid_counts, invalid_receive) != expected_zero_layout ||
        context.AllGatherCounts(0, invalid_receive) != expected_zero_layout) {
        return false;
    }

    const std::array<blitzar_parallel::ParticlePacket, 0> empty_packets{};

    const blitzar_parallel::AllToAllPacketRequest invalid_request{empty_packets, invalid_counts,
        invalid_counts, std::span<blitzar_parallel::ParticlePacket>{}, invalid_counts,
        invalid_counts};

    if (context.AllToAllPackets(invalid_request) != expected_zero_layout) {
        return false;
    }

    const std::array<double, 2> invalid_minimum{};

    const std::array<double, 3> invalid_maximum{};
    std::array<double, 2> minimum = invalid_minimum;
    std::array<double, 3> maximum = invalid_maximum;

    return context.ReduceBounds(minimum, maximum) == BLITZAR_STATUS_INVALID_ARGUMENT;
}

[[nodiscard]] bool RunLargeCountValidationCase(const blitzar_parallel::MpiContext& context) noexcept
{
    std::array<int, 4> counts{};
    std::array<int, 4> displacements{};

    counts.fill(std::numeric_limits<int>::max());

    const std::span<const int> layout =
        std::span<const int>(counts).first(static_cast<std::size_t>(context.Size()));

    const std::span<const int> offsets =
        std::span<const int>(displacements).first(static_cast<std::size_t>(context.Size()));

    const std::span<blitzar_parallel::ParticlePacket> empty_packets{};

    const blitzar_parallel::AllToAllPacketRequest request{
        empty_packets, layout, offsets, empty_packets, layout, offsets};

    return context.AllToAllPackets(request) == BLITZAR_STATUS_INVALID_ARGUMENT;
}

[[nodiscard]] bool RunWireCodecCase() noexcept
{
    blitzar_parallel::PacketBuffer bounded_packets;

    bounded_packets.Reserve(2);

    if (!bounded_packets.ResizeBounded(2) || bounded_packets.ResizeBounded(3) ||
        bounded_packets.Size() != 2) {
        return false;
    }

    const blitzar_parallel::ParticlePacket source{
        0x0102030405060708ULL, 1.0, -2.5, 3.75, -4.5, 5.25, -6.75, 7.5};

    blitzar_parallel::ParticleWire wire{};

    if (!blitzar_parallel::ParticleWireCodec::Encode(source, wire)) {
        return false;
    }

    const std::array<unsigned int, 8> expected_id_bytes{
        0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01};

    for (std::size_t index = 0; index < expected_id_bytes.size(); ++index) {
        if (wire[index] != static_cast<std::byte>(expected_id_bytes[index])) {
            return false;
        }
    }

    const std::array<unsigned int, 8> expected_one_bytes{
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x3f};

    for (std::size_t index = 0; index < expected_one_bytes.size(); ++index) {
        if (wire[8 + index] != static_cast<std::byte>(expected_one_bytes[index])) {
            return false;
        }
    }

    blitzar_parallel::ParticlePacket decoded{};

    if (!blitzar_parallel::ParticleWireCodec::Decode(wire, decoded) || decoded.id != source.id) {
        return false;
    }

    const std::array<double, 7> source_scalars{source.x, source.y, source.z, source.velocity_x,
        source.velocity_y, source.velocity_z, source.mass};

    const std::array<double, 7> decoded_scalars{decoded.x, decoded.y, decoded.z, decoded.velocity_x,
        decoded.velocity_y, decoded.velocity_z, decoded.mass};

    for (std::size_t index = 0; index < source_scalars.size(); ++index) {
        if (std::bit_cast<std::uint64_t>(source_scalars[index]) !=
            std::bit_cast<std::uint64_t>(decoded_scalars[index])) {
            return false;
        }
    }

    std::array<std::byte, blitzar_parallel::ParticleWireBytes - 1> short_wire{};

    return !blitzar_parallel::ParticleWireCodec::Encode(source, short_wire) &&
           !blitzar_parallel::ParticleWireCodec::Decode(short_wire, decoded);
}

[[nodiscard]] std::uint64_t PeakResidentBytes() noexcept
{
#if defined(__linux__)
    struct rusage usage{};

    if (getrusage(RUSAGE_SELF, &usage) != 0 || usage.ru_maxrss < 0) {
        return 0;
    }

    return static_cast<std::uint64_t>(usage.ru_maxrss) * 1024;
#else
    return 0;
#endif
}

void ReportPeakResidentMemory(
    const blitzar_parallel::MpiContext& context, std::size_t particle_count) noexcept
{
    const std::uint64_t bytes = PeakResidentBytes();

    if (bytes != 0) {
        std::fprintf(stderr,
            "BLITZAR MPI memory rank=%d particles=%zu ranks=%d peak_rss_bytes=%llu\n",
            context.Rank(), particle_count, context.Size(), static_cast<unsigned long long>(bytes));
    }
}

} // namespace

int RunTests(int argc, char** argv)
{
    blitzar_parallel::MpiContext context;
    const std::string_view mode = argc > 1 ? argv[1] : std::string_view{};
    const bool single_rank_case = mode == "single";
    const bool migration_case = mode == "migration" || mode == "barnes-hut-migration";
    const bool barnes_hut_case = mode == "barnes-hut-migration";
    const bool out_of_domain_case = mode == "out-of-domain";
    const bool large_count_case = mode == "large-count";
    const bool overlap_case = mode == "overlap";
    const bool valid_world =
        context.IsUsable() &&
        (single_rank_case ? context.Size() == 1 : (context.Size() == 2 || context.Size() == 4));

    const bool local_case =
        RunCase(migration_case ? MigrationState() : InitialState(), 0.01, migration_case ? 1 : 2,
            barnes_hut_case ? BLITZAR_SOLVER_BARNES_HUT : BLITZAR_SOLVER_DIRECT);

    const bool allocation_case = RunAllocationCase();

    const bool rollback_case = RunRollbackCase();
    const bool boundary_case = RunBoundaryOwnershipCase(context);
    const bool error_synchronization_case = RunErrorSynchronizationCase(context);
    const bool nested_context_case = RunNestedContextCase(context);
    const bool collective_validation_case = RunCollectiveValidationCase(context);
    const bool out_of_domain_result = !out_of_domain_case || RunOutOfDomainCase();
    const bool large_count_result = !large_count_case || RunLargeCountValidationCase(context);
    const bool overlap_result = !overlap_case || RunOverlapCase(context);
    const bool wire_codec_case = RunWireCodecCase();
    const bool local_ok = valid_world && local_case && allocation_case && rollback_case &&
                          boundary_case && error_synchronization_case && nested_context_case &&
                          collective_validation_case && out_of_domain_result &&
                          large_count_result && overlap_result && wire_codec_case;

    ReportPeakResidentMemory(context, ParticleCount);

    if (!local_ok) {
        std::fprintf(stderr,
            "MPI test failure rank=%d size=%d valid_world=%d local=%d "
            "allocation=%d rollback=%d boundary=%d error_sync=%d nested=%d collective=%d "
            "out_of_domain=%d large_count=%d overlap=%d wire=%d\n",
            context.Rank(), context.Size(), valid_world, local_case, allocation_case, rollback_case,
            boundary_case, error_synchronization_case, nested_context_case,
            collective_validation_case, out_of_domain_result, large_count_result, overlap_result,
            wire_codec_case);
    }

    int local_failure = local_ok ? 0 : 1;
    int global_failure = 0;

    BLITZAR_CHECK(context.ReduceMax(local_failure, global_failure) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(global_failure == 0);

    return 0;
}

int main(int argc, char** argv)
{
#if defined(BLITZAR_HAS_MPI)
    const std::string_view mode = argc > 1 ? argv[1] : std::string_view{};
    const bool internal_owner = mode == "internal";
    int external_owner = 0;

    if (!internal_owner) {
        int initialized = 0;

        if (MPI_Initialized(&initialized) != MPI_SUCCESS) {
            return 1;
        }
        if (initialized == 0) {
            int provided = MPI_THREAD_SINGLE;

            if (MPI_Init_thread(nullptr, nullptr, MPI_THREAD_MULTIPLE, &provided) != MPI_SUCCESS ||
                provided < MPI_THREAD_MULTIPLE) {
                return 1;
            }

            external_owner = 1;
        }
    }
#endif

    const int result = RunTests(argc, argv);

#if defined(BLITZAR_HAS_MPI)

    if (external_owner != 0) {
        int finalized = 0;

        if (MPI_Finalized(&finalized) != MPI_SUCCESS || finalized != 0 ||
            MPI_Finalize() != MPI_SUCCESS) {
            return 1;
        }
    }
#endif

    return result;
}
