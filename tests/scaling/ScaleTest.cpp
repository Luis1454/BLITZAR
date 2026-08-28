#include "ScaleTest.hpp"

#include <charconv>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>
#include <system_error>

namespace {

struct Arguments final {
    blitzar_scaling::Config config{};
    bool help{false};
};

template <typename Value> bool ParseInteger(std::string_view text, Value& value) noexcept
{
    Value parsed{};
    const auto result = std::from_chars(text.data(), text.data() + text.size(), parsed);

    if (result.ec != std::errc{} || result.ptr != text.data() + text.size()) {
        return false;
    }

    value = parsed;

    return true;
}

bool ParseReal(std::string_view text, double& value)
{
    const std::string storage(text);

    char* end = nullptr;

    const double parsed = std::strtod(storage.c_str(), &end);

    if (end != storage.c_str() + storage.size()) {
        return false;
    }

    value = parsed;

    return true;
}

bool NextValue(int argc, char** argv, int& index, std::string_view& value) noexcept
{
    if (index + 1 >= argc) {
        return false;
    }

    value = argv[++index];

    return true;
}

bool ParseSolver(std::string_view text, blitzar_scaling::SolverKind& solver) noexcept
{
    if (text == "direct") {
        solver = blitzar_scaling::SolverKind::Direct;

        return true;
    }
    if (text == "barnes-hut") {
        solver = blitzar_scaling::SolverKind::BarnesHut;

        return true;
    }
    if (text == "fmm") {
        solver = blitzar_scaling::SolverKind::Fmm;

        return true;
    }

    return false;
}

bool ParseOverlap(std::string_view text, blitzar_scaling::OverlapMode& mode) noexcept
{
    if (text == "overlapped") {
        mode = blitzar_scaling::OverlapMode::Overlapped;

        return true;
    }
    if (text == "serialized") {
        mode = blitzar_scaling::OverlapMode::Serialized;

        return true;
    }

    return false;
}

bool ParseDistribution(
    std::string_view text, blitzar_scaling::DistributionKind& distribution) noexcept
{
    if (text == "box-pair-v1") {
        distribution = blitzar_scaling::DistributionKind::BoxPair;

        return true;
    }
    if (text == "boundary-crossing-v1") {
        distribution = blitzar_scaling::DistributionKind::BoundaryCrossing;

        return true;
    }

    return false;
}

bool ParseArguments(int argc, char** argv, Arguments& arguments)
{
    for (int index = 1; index < argc; ++index) {
        const std::string_view option = argv[index];

        if (option == "--help") {
            arguments.help = true;

            continue;
        }
        if (option == "--oracle") {
            arguments.config.oracle = true;

            continue;
        }
        if (option == "--migration") {
            arguments.config.migration = true;
            arguments.config.distribution = blitzar_scaling::DistributionKind::BoundaryCrossing;

            continue;
        }

        std::string_view value;

        if (!NextValue(argc, argv, index, value)) {
            return false;
        }
        if (option == "--particles" && !ParseInteger(value, arguments.config.particle_count)) {
            return false;
        }
        else if (option == "--warmup" && !ParseInteger(value, arguments.config.warmup_steps)) {
            return false;
        }
        else if (option == "--steps" && !ParseInteger(value, arguments.config.timed_steps)) {
            return false;
        }
        else if (option == "--seed" && !ParseInteger(value, arguments.config.seed)) {
            return false;
        }
        else if (option == "--tolerance" && !ParseReal(value, arguments.config.oracle_tolerance)) {
            return false;
        }
        else if (option == "--solver" && !ParseSolver(value, arguments.config.solver)) {
            return false;
        }
        else if (option == "--overlap" && !ParseOverlap(value, arguments.config.overlap)) {
            return false;
        }
        else if (option == "--distribution" &&
                 !ParseDistribution(value, arguments.config.distribution)) {
            return false;
        }
        else if (option != "--particles" && option != "--warmup" && option != "--steps" &&
                 option != "--seed" && option != "--tolerance" && option != "--solver" &&
                 option != "--overlap" && option != "--distribution") {
            return false;
        }
    }

    return arguments.config.particle_count > 0 && arguments.config.particle_count <= 1000000 &&
           arguments.config.warmup_steps >= 0 && arguments.config.timed_steps > 0 &&
           std::isfinite(arguments.config.oracle_tolerance) &&
           arguments.config.oracle_tolerance >= 0.0 &&
           (!arguments.config.migration || arguments.config.distribution ==
                                               blitzar_scaling::DistributionKind::BoundaryCrossing);
}

void PrintUsage() noexcept
{
    std::fprintf(stdout,
        "usage: blitzar_scaling_test [--particles N] [--warmup N] [--steps N] "
        "[--seed N] [--solver direct|barnes-hut|fmm] "
        "[--overlap overlapped|serialized] [--distribution box-pair-v1|boundary-crossing-v1] "
        "[--tolerance X] [--oracle] [--migration]\n");
}

void PrintResult(
    const blitzar_scaling::Config& config, const blitzar_scaling::Result& result) noexcept
{
    const auto& overlap = result.overlap_trace;
    const auto& migration = result.migration_trace;

    std::fprintf(stdout,
        "BLITZAR SCALE schema=2 rank=%d ranks=%d particles=%zu warmup_steps=%d "
        "timed_steps=%d seed=%llu distribution=%.*s solver=%.*s overlap=%.*s "
        "status=%d backend=%d elapsed_ns=%llu mean_step_ns=%llu min_step_ns=%llu "
        "max_step_ns=%llu allocation_count=%zu allocation_free=%d peak_rss_bytes=%llu "
        "throughput_particles_per_second=%.17g local_before=%zu local_after=%zu "
        "migration_observed=%d migration_sent_remote=%zu migration_received_remote=%zu "
        "overlap_has_overlap=%d local_packets=%zu ghost_packets=%zu send_bytes=%zu "
        "receive_bytes=%zu oracle_checked=%d oracle_pass=%d oracle_max_error=%.17g\n",
        result.rank, result.ranks, config.particle_count, config.warmup_steps, config.timed_steps,
        static_cast<unsigned long long>(config.seed),
        static_cast<int>(blitzar_scaling::DistributionName(config.distribution).size()),
        blitzar_scaling::DistributionName(config.distribution).data(),
        static_cast<int>(blitzar_scaling::SolverName(config.solver).size()),
        blitzar_scaling::SolverName(config.solver).data(),
        static_cast<int>(blitzar_scaling::OverlapName(config.overlap).size()),
        blitzar_scaling::OverlapName(config.overlap).data(), static_cast<int>(result.status),
        static_cast<int>(result.backend), static_cast<unsigned long long>(result.elapsed_ns),
        static_cast<unsigned long long>(result.mean_step_ns),
        static_cast<unsigned long long>(result.min_step_ns),
        static_cast<unsigned long long>(result.max_step_ns), result.allocation_count,
        result.allocation_count == 0 ? 1 : 0,
        static_cast<unsigned long long>(result.peak_rss_bytes),
        result.throughput_particles_per_second, result.local_before, result.local_after,
        migration.observed ? 1 : 0, migration.sent_remote, migration.received_remote,
        overlap.HasOverlap() ? 1 : 0, overlap.local_packets, overlap.ghost_packets,
        overlap.send_bytes, overlap.receive_bytes, result.oracle_checked ? 1 : 0,
        result.oracle_pass ? 1 : 0, result.oracle_max_error);
}

} // namespace

int main(int argc, char** argv)
{
    Arguments arguments;

    if (!ParseArguments(argc, argv, arguments)) {
        PrintUsage();

        return 2;
    }
    if (arguments.help) {
        PrintUsage();

        return 0;
    }

    blitzar_scaling::Result result;
    const bool passed = blitzar_scaling::Run(arguments.config, result);

    PrintResult(arguments.config, result);

    return passed ? 0 : 1;
}
