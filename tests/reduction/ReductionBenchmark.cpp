#include "reduction/ReductionBenchmark.hpp"

#include "core/CoreExecution.hpp"
#include "integration/kdk/KdkLeapfrog.hpp"
#include "particles/buffer/ParticleAccelerationBuffer.hpp"
#include "particles/buffer/ParticleBuffer.hpp"
#include "physics/conservation/ConservationMetrics.hpp"
#include "physics/gravity/GravityLaw.hpp"
#include "solvers/SolverCpuForceProvider.hpp"
#include "solvers/direct/DirectSolver.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace blitzar_reduction {

namespace {

constexpr std::array<blitzar_physics::ReductionKind, 3> ReductionKinds{
    blitzar_physics::ReductionKind::Plain, blitzar_physics::ReductionKind::Kahan,
    blitzar_physics::ReductionKind::Neumaier};

struct Workload final {
    std::vector<double> terms;
    double expected{};
};

Workload MakeWorkload(WorkloadKind kind, std::size_t term_count)
{
    Workload workload{};

    if (term_count == 0 || term_count % 4 != 0) {
        return workload;
    }

    workload.terms.resize(term_count);

    std::array<double, 4> pattern{};

    switch (kind) {
    case WorkloadKind::Force:

        pattern = {1.0e16, 1.0, -1.0e16, 0.0};
        workload.expected = static_cast<double>(term_count / 4);

        break;

    case WorkloadKind::Kinetic:

        pattern = {1.0e16, 1.0, -1.0e16, 1.0};
        workload.expected = static_cast<double>(term_count / 2);

        break;

    case WorkloadKind::Potential:

        pattern = {-1.0e16, -1.0, 1.0e16, 0.0};
        workload.expected = -static_cast<double>(term_count / 4);

        break;

    case WorkloadKind::Momentum:

        pattern = {1.0e16, 0.5, -1.0e16, 0.25};
        workload.expected = 0.75 * static_cast<double>(term_count / 4);

        break;
    }

    for (std::size_t index = 0; index < term_count; ++index) {
        workload.terms[index] = pattern[index % pattern.size()];
    }

    return workload;
}

std::uint64_t Hash(std::span<const double> values) noexcept
{
    std::uint64_t hash = 1469598103934665603ULL;

    for (const double value : values) {
        hash ^= std::bit_cast<std::uint64_t>(value);
        hash *= 1099511628211ULL;
    }

    return hash;
}

std::uint64_t HashState(blitzar_core::ParticleStateView state) noexcept
{
    std::uint64_t hash = 1469598103934665603ULL;
    const auto append = [&hash](const double value) noexcept {
        hash ^= std::bit_cast<std::uint64_t>(value);
        hash *= 1099511628211ULL;
    };

    for (std::size_t index = 0; index < state.count; ++index) {
        append(state.x[index]);
        append(state.y[index]);
        append(state.z[index]);
        append(state.velocity_x[index]);
        append(state.velocity_y[index]);
        append(state.velocity_z[index]);
        append(state.mass[index]);
    }

    return hash;
}

std::uint64_t Elapsed(
    std::chrono::steady_clock::time_point begin, std::chrono::steady_clock::time_point end) noexcept
{
    const auto value = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();

    return value > 0 ? static_cast<std::uint64_t>(value) : 1U;
}

bool InitializeOrbit(blitzar_particles::ParticleBuffer& particles) noexcept
{
    const double softening = 0.05;
    const double speed = std::sqrt(0.5 / std::pow(1.0 + softening * softening, 1.5));

    return particles.SetPosition(0, {-0.5, 0.0, 0.0}) == BLITZAR_STATUS_OK &&
           particles.SetPosition(1, {0.5, 0.0, 0.0}) == BLITZAR_STATUS_OK &&
           particles.SetVelocity(0, {0.0, speed, 0.0}) == BLITZAR_STATUS_OK &&
           particles.SetVelocity(1, {0.0, -speed, 0.0}) == BLITZAR_STATUS_OK &&
           particles.SetMass(0, 1.0) == BLITZAR_STATUS_OK &&
           particles.SetMass(1, 1.0) == BLITZAR_STATUS_OK;
}

double RelativeError(double value, double reference) noexcept
{
    const double scale = std::max(std::abs(reference), 1.0);

    return std::abs(value - reference) / scale;
}

bool SameMetrics(const blitzar_physics::ConservationMetrics& first,
    const blitzar_physics::ConservationMetrics& second) noexcept
{
    return std::bit_cast<std::uint64_t>(first.kinetic_energy) ==
               std::bit_cast<std::uint64_t>(second.kinetic_energy) &&
           std::bit_cast<std::uint64_t>(first.potential_energy) ==
               std::bit_cast<std::uint64_t>(second.potential_energy) &&
           std::bit_cast<std::uint64_t>(first.total_energy) ==
               std::bit_cast<std::uint64_t>(second.total_energy) &&
           std::bit_cast<std::uint64_t>(first.momentum.x) ==
               std::bit_cast<std::uint64_t>(second.momentum.x) &&
           std::bit_cast<std::uint64_t>(first.momentum.y) ==
               std::bit_cast<std::uint64_t>(second.momentum.y) &&
           std::bit_cast<std::uint64_t>(first.momentum.z) ==
               std::bit_cast<std::uint64_t>(second.momentum.z);
}

} // namespace

ReductionBenchmark::ReductionBenchmark(std::uint64_t seed) : seed_(seed) {}

bool ReductionBenchmark::Run(std::span<const std::size_t> term_counts,
    std::vector<ReductionResult>& results, std::vector<LongRunResult>& long_runs) const
{
    if (term_counts.empty() || term_counts.size() > std::numeric_limits<std::size_t>::max() / 12U) {
        return false;
    }

    results.clear();
    results.reserve(term_counts.size() * 12U);
    long_runs.clear();
    long_runs.reserve(ReductionKinds.size());

    for (const std::size_t term_count : term_counts) {
        if (!RunCount(term_count, results)) {
            return false;
        }
    }

    return RunLongRun(long_runs);
}

bool ReductionBenchmark::RunCount(
    std::size_t term_count, std::vector<ReductionResult>& results) const
{
    if (term_count == 0 || term_count % 4 != 0) {
        return false;
    }

    constexpr std::array<WorkloadKind, 4> workloads{WorkloadKind::Force, WorkloadKind::Kinetic,
        WorkloadKind::Potential, WorkloadKind::Momentum};

    for (const WorkloadKind workload_kind : workloads) {
        const Workload workload = MakeWorkload(workload_kind, term_count);

        if (!Measure(workload_kind, workload.terms, workload.expected, results)) {
            return false;
        }
    }

    return true;
}

bool ReductionBenchmark::Measure(WorkloadKind workload, std::span<const double> terms,
    double expected, std::vector<ReductionResult>& results) const
{
    const std::uint64_t input_hash = Hash(terms);

    for (const blitzar_physics::ReductionKind reduction : ReductionKinds) {
        blitzar_physics::ScalarReduction first(reduction);
        const auto begin = std::chrono::steady_clock::now();

        for (const double term : terms) {
            first.Add(term);
        }

        const auto end = std::chrono::steady_clock::now();
        blitzar_physics::ScalarReduction repeat(reduction);

        for (const double term : terms) {
            repeat.Add(term);
        }

        const double value = first.Value();
        const double repeated_value = repeat.Value();
        const std::uint64_t elapsed_ns = Elapsed(begin, end);
        const bool finite = std::isfinite(value) && std::isfinite(expected);
        const bool repeatable =
            std::bit_cast<std::uint64_t>(value) == std::bit_cast<std::uint64_t>(repeated_value);

        const double absolute_error = finite ? std::abs(value - expected) : 0.0;

        results.push_back({seed_, terms.size(), workload, reduction, elapsed_ns,
            static_cast<double>(terms.size()) * 1.0e9 / static_cast<double>(elapsed_ns), expected,
            value, absolute_error, finite ? RelativeError(value, expected) : 0.0, input_hash,
            std::bit_cast<std::uint64_t>(value), repeatable, finite,
            reduction == blitzar_physics::ReductionKind::Plain,
            workload == WorkloadKind::Force
                ? reduction == blitzar_physics::ReductionKind::Plain
                : reduction == blitzar_physics::ReductionKind::Neumaier});
    }

    return true;
}

bool ReductionBenchmark::RunLongRun(std::vector<LongRunResult>& results) const
{
    constexpr std::size_t steps = 4096;
    constexpr double timestep = 0.001;
    const blitzar_physics::GravityParameters gravity{1.0, 0.05};
    blitzar_particles::ParticleBuffer particles(2);
    blitzar_particles::ParticleAccelerationBuffer accelerations(2);
    blitzar_integration::KdkCheckpoint checkpoint(2);
    blitzar_direct::DirectSolver solver(gravity);
    blitzar_solvers::SolverCpuForceProvider<blitzar_direct::DirectSolver> provider(solver);

    if (!InitializeOrbit(particles) || solver.Prepare(2) != BLITZAR_STATUS_OK) {
        return false;
    }

    const blitzar_core::ExecutionSettings settings{};
    const blitzar_integration::KdkLeapfrog integrator{};
    blitzar_integration_kdk::AdvanceState<decltype(provider)> state{
        particles, accelerations, checkpoint, provider, timestep, settings, particles.State()};

    std::array<blitzar_physics::ConservationMetrics, ReductionKinds.size()> initial{};
    std::array<blitzar_physics::ConservationMetrics, ReductionKinds.size()> final{};
    std::array<double, ReductionKinds.size()> initial_energy{};
    std::array<double, ReductionKinds.size()> maximum_error{};

    for (std::size_t index = 0; index < ReductionKinds.size(); ++index) {
        if (blitzar_physics::ComputeConservationMetrics(particles.State(), gravity,
                ReductionKinds[index], initial[index]) != BLITZAR_STATUS_OK) {
            return false;
        }

        initial_energy[index] = initial[index].total_energy;
    }

    for (std::size_t step = 0; step < steps; ++step) {
        if (integrator.Advance(state) != BLITZAR_STATUS_OK) {
            return false;
        }

        for (std::size_t index = 0; index < ReductionKinds.size(); ++index) {
            blitzar_physics::ConservationMetrics current{};

            if (blitzar_physics::ComputeConservationMetrics(particles.State(), gravity,
                    ReductionKinds[index], current) != BLITZAR_STATUS_OK) {
                return false;
            }

            final[index] = current;
            maximum_error[index] = std::max(
                maximum_error[index], RelativeError(current.total_energy, initial_energy[index]));
        }
    }

    blitzar_physics::ConservationMetrics default_metrics{};

    if (blitzar_physics::ComputeConservationMetrics(particles.State(), gravity, default_metrics) !=
        BLITZAR_STATUS_OK) {
        return false;
    }

    blitzar_physics::ConservationMetrics selected_metrics{};

    if (blitzar_physics::ComputeConservationMetrics(particles.State(), gravity,
            blitzar_physics::ReductionKind::Neumaier, selected_metrics) != BLITZAR_STATUS_OK) {
        return false;
    }

    const bool default_policy_match = SameMetrics(default_metrics, selected_metrics);

    for (std::size_t index = 0; index < ReductionKinds.size(); ++index) {
        const auto reduction = ReductionKinds[index];

        const auto& metrics = final[index];

        const double momentum_norm = std::sqrt(metrics.momentum.x * metrics.momentum.x +
                                               metrics.momentum.y * metrics.momentum.y +
                                               metrics.momentum.z * metrics.momentum.z);

        results.push_back({seed_, steps, reduction, maximum_error[index], metrics.total_energy,
            momentum_norm, HashState(particles.State()), metrics.IsFinite(), default_policy_match,
            reduction == blitzar_physics::ReductionKind::Neumaier});
    }

    return true;
}

} // namespace blitzar_reduction
