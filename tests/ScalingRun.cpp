#include "Scaling.hpp"
#include "integration/KdkCheckpoint.hpp"
#include "particles/AccelerationBuffer.hpp"
#include "particles/ParticleBuffer.hpp"
#include "solvers/direct/DirectSolver.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <new>
#include <span>

#if defined(_WIN32)
// MinGW's PSAPI declarations require the Win32 base types first.
// clang-format off
#include <windows.h>
#include <psapi.h>
// clang-format on
#elif defined(__linux__) || defined(__APPLE__)
#include <sys/resource.h>
#endif

namespace blitzar_scaling {

namespace {

std::uint64_t PeakResidentBytes() noexcept
{
#if defined(_WIN32)
    PROCESS_MEMORY_COUNTERS counters{};

    if (!GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters))) {
        return 0;
    }

    return static_cast<std::uint64_t>(counters.PeakWorkingSetSize);
#elif defined(__linux__)

    struct rusage usage{};

    if (getrusage(RUSAGE_SELF, &usage) != 0 || usage.ru_maxrss < 0) {
        return 0;
    }

    return static_cast<std::uint64_t>(usage.ru_maxrss) * 1024U;
#elif defined(__APPLE__)

    struct rusage usage{};

    if (getrusage(RUSAGE_SELF, &usage) != 0 || usage.ru_maxrss < 0) {
        return 0;
    }

    return static_cast<std::uint64_t>(usage.ru_maxrss);
#else
    return 0;
#endif
}

bool CopyToParticles(const State& state, blitzar_particles::ParticleBuffer& particles) noexcept
{
    for (std::size_t index = 0; index < state.x.size(); ++index) {
        if (particles.SetPosition(index, {state.x[index], state.y[index], state.z[index]}) !=

                BLITZAR_STATUS_OK ||
            particles.SetVelocity(index, {state.velocity_x[index], state.velocity_y[index],
                                             state.velocity_z[index]}) != BLITZAR_STATUS_OK ||
            particles.SetMass(index, state.mass[index]) != BLITZAR_STATUS_OK) {
            return false;
        }
    }

    return true;
}

void CopyFromParticles(const blitzar_particles::ParticleBuffer& particles, State& state) noexcept
{
    const blitzar_core::ParticleStateView view = particles.State();

    std::copy(view.x.begin(), view.x.end(), state.x.begin());
    std::copy(view.y.begin(), view.y.end(), state.y.begin());
    std::copy(view.z.begin(), view.z.end(), state.z.begin());
    std::copy(view.velocity_x.begin(), view.velocity_x.end(), state.velocity_x.begin());
    std::copy(view.velocity_y.begin(), view.velocity_y.end(), state.velocity_y.begin());
    std::copy(view.velocity_z.begin(), view.velocity_z.end(), state.velocity_z.begin());
    std::copy(view.mass.begin(), view.mass.end(), state.mass.begin());
}

void MergeMigrationTrace(const blitzar_parallel::MpiMigrationTrace& current,
    blitzar_parallel::MpiMigrationTrace& aggregate) noexcept;

blitzar_status RunTimedSteps(blitzar_sdk::Simulation& simulation, const Config& config,
    std::uint64_t& elapsed_ns, blitzar_parallel::MpiMigrationTrace& migration_trace) noexcept
{
    migration_trace = {};

    for (int step = 0; step < config.warmup_steps; ++step) {
        const blitzar_status status = simulation.Step();

        MergeMigrationTrace(simulation.LastMpiMigrationTrace(), migration_trace);

        if (status != BLITZAR_STATUS_OK) {
            return status;
        }
    }

    const auto start = std::chrono::steady_clock::now();

    for (int step = 0; step < config.timed_steps; ++step) {
        const blitzar_status status = simulation.Step();

        MergeMigrationTrace(simulation.LastMpiMigrationTrace(), migration_trace);

        if (status != BLITZAR_STATUS_OK) {
            return status;
        }
    }

    const auto end = std::chrono::steady_clock::now();

    elapsed_ns = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count());

    return BLITZAR_STATUS_OK;
}

bool RunCpuOracle(const Config& config, const State& input, State& expected)
{
    blitzar_particles::ParticleBuffer particles(config.particle_count);
    blitzar_particles::AccelerationBuffer accelerations(config.particle_count);
    blitzar_integration::KdkCheckpoint checkpoint(config.particle_count);
    const blitzar_physics::GravityParameters gravity{1.0, 0.05};
    blitzar_direct::DirectSolver solver(gravity, config.particle_count);

    if (!CopyToParticles(input, particles) ||
        solver.Prepare(config.particle_count) != BLITZAR_STATUS_OK) {
        return false;
    }

    const blitzar_core::ExecutionSettings settings{
        config.seed, blitzar_core::ExecutionMode::Deterministic};

    std::span<std::size_t> scratch;
    blitzar_integration_kdk::AdvanceState state{
        particles, accelerations, checkpoint, solver, 0.001, settings, scratch, particles.State()};

    const int step_count = config.warmup_steps + config.timed_steps;

    for (int step = 0; step < step_count; ++step) {
        if (blitzar_integration::LeapfrogKdk{}.Advance(state) != BLITZAR_STATUS_OK) {
            return false;
        }
    }

    CopyFromParticles(particles, expected);

    return true;
}

double MaxStateError(const State& actual, const State& expected) noexcept
{
    double maximum = 0.0;

    for (std::size_t index = 0; index < actual.x.size(); ++index) {
        maximum = std::max(maximum, std::abs(actual.x[index] - expected.x[index]));
        maximum = std::max(maximum, std::abs(actual.y[index] - expected.y[index]));
        maximum = std::max(maximum, std::abs(actual.z[index] - expected.z[index]));
        maximum =
            std::max(maximum, std::abs(actual.velocity_x[index] - expected.velocity_x[index]));

        maximum =
            std::max(maximum, std::abs(actual.velocity_y[index] - expected.velocity_y[index]));

        maximum =
            std::max(maximum, std::abs(actual.velocity_z[index] - expected.velocity_z[index]));

        maximum = std::max(maximum, std::abs(actual.mass[index] - expected.mass[index]));
    }

    return maximum;
}

void MergeMigrationTrace(const blitzar_parallel::MpiMigrationTrace& current,
    blitzar_parallel::MpiMigrationTrace& aggregate) noexcept
{
    if (aggregate.local_before == 0 && aggregate.local_after == 0 && !aggregate.observed) {
        aggregate.local_before = current.local_before;
    }

    aggregate.status = current.status;
    aggregate.local_after = current.local_after;
    aggregate.sent_remote += current.sent_remote;
    aggregate.received_remote += current.received_remote;
    aggregate.observed = aggregate.observed || current.observed;
}

} // namespace

bool Run(const Config& config, Result& result)
{
    result = {};

    try {
        blitzar_sdk::Simulation simulation(config.particle_count);

        result.rank = simulation.MpiRank();
        result.ranks = simulation.MpiSize();

        const State input =
            MakeState(result.rank == 0 ? config.particle_count : 0, config.seed, config.migration);

        State output(config.particle_count);

        if (!Configure(simulation, config, input)) {
            result.status = simulation.LastStatus();

            return false;
        }

        result.local_before = simulation.LocalParticleCount();

        const blitzar_status step_status =
            RunTimedSteps(simulation, config, result.elapsed_ns, result.migration_trace);

        result.status = step_status;
        result.local_after = simulation.LocalParticleCount();
        result.backend = simulation.LastBackend();
        result.overlap_trace = simulation.LastMpiOverlapTrace();
        result.peak_rss_bytes = PeakResidentBytes();

        if (step_status != BLITZAR_STATUS_OK ||
            simulation.GetState(OutputView(output)) != BLITZAR_STATUS_OK) {
            result.status = simulation.LastStatus();

            return false;
        }

        if (config.oracle && config.solver != SolverKind::Direct && !config.migration &&
            result.rank == 0) {
            State expected(config.particle_count);

            result.oracle_checked = true;
            result.oracle_pass = RunCpuOracle(config, input, expected);
            result.oracle_max_error = result.oracle_pass ? MaxStateError(output, expected) : 0.0;
            result.oracle_pass =
                result.oracle_pass && result.oracle_max_error <= config.oracle_tolerance;
        }

        return !result.oracle_checked || result.oracle_pass;
    }
    catch (const std::bad_alloc&) {
        result.status = BLITZAR_STATUS_ALLOCATION_FAILURE;

        return false;
    }
    catch (...) {
        result.status = BLITZAR_STATUS_INTERNAL_ERROR;

        return false;
    }
}

} // namespace blitzar_scaling
