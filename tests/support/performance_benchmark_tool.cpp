// File: tests/support/performance_benchmark_tool.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "tests/support/performance_benchmark_tool.hpp"
#include "tests/support/physics_scenario.hpp"
#include "tests/support/physics_test_utils.hpp"
#include <chrono>
#include <iomanip>
#include <ostream>

namespace grav_test_perf_tool {
/// Description: Describes the build scenario operation contract.
bool buildScenario(const grav_test_perf::PerformanceBenchmarkTool::ToolOptions& options,
                   testsupport::ScenarioConfig& cfg, std::string& error)
{
    if (options.workload == "disk_orbit") {
        cfg = testsupport::buildDiskOrbitScenario(options.particleCount, options.dt, options.steps,
                                                  options.seed, options.solver, options.integrator);
    }
    else if (options.workload == "random_cloud") {
        cfg =
            testsupport::buildRandomCloudScenario(options.particleCount, options.dt, options.steps,
                                                  options.seed, options.solver, options.integrator);
    }
    else {
        error = "unknown workload: " + options.workload;
        return false;
    }
    testsupport::setScenarioTiming(cfg, 30000, 30000);
    testsupport::setScenarioEnergySampling(cfg, 1u, options.particleCount);
    return true;
}

/// Description: Describes the write measurement operation contract.
void writeMeasurement(const grav_test_perf::PerformanceBenchmarkTool::ToolOptions& options,
                      const testsupport::ScenarioResult& result, const double wallSeconds,
                      std::ostream& out)
{
    const double steps = static_cast<double>(options.steps);
    const double particleUpdates = steps * static_cast<double>(result.final.size());
    const double safeWallSeconds = wallSeconds > 1e-9 ? wallSeconds : 1e-9;
    out << std::fixed << std::setprecision(8);
    out << "workload=" << options.workload << "\n";
    out << "solver=" << options.solver << "\n";
    out << "integrator=" << options.integrator << "\n";
    out << "dt=" << options.dt << "\n";
    out << "particle_count=" << result.final.size() << "\n";
    out << "steps=" << options.steps << "\n";
    out << "seed=" << options.seed << "\n";
    out << "wall_seconds=" << wallSeconds << "\n";
    out << "steps_per_second=" << (steps / safeWallSeconds) << "\n";
    out << "particles_per_second=" << (particleUpdates / safeWallSeconds) << "\n";
    out << "average_step_ms=" << (wallSeconds * 1000.0 / steps) << "\n";
    out << "total_time=" << result.stats.totalTime << "\n";
    out << "server_fps_final=" << result.stats.serverFps << "\n";
    out << "energy_drift_pct=" << result.stats.energyDriftPct << "\n";
}
} // namespace grav_test_perf_tool

namespace grav_test_perf {
/// Description: Executes the parseFloatValue operation.
bool PerformanceBenchmarkTool::parseFloatValue(const std::string& text, float& out)
{
    std::size_t consumed = 0u;
    try {
        out = std::stof(text, &consumed);
    }
    catch (...) {
        return false;
    }
    return consumed == text.size();
}

/// Description: Executes the parseUintValue operation.
bool PerformanceBenchmarkTool::parseUintValue(const std::string& text, std::uint32_t& out)
{
    std::size_t consumed = 0u;
    try {
        out = static_cast<std::uint32_t>(std::stoul(text, &consumed, 10));
    }
    catch (...) {
        return false;
    }
    return consumed == text.size();
}

/// Description: Describes the parse args operation contract.
bool PerformanceBenchmarkTool::parseArgs(int argc, const char* const* argv, ToolOptions& out,
                                         std::string& error)
{
    for (int index = 1; index < argc; index += 1) {
        const std::string arg(argv[index]);
        if (arg == "--help") {
            error = "usage: gravityPerformanceBenchmarkTool --workload <name> --solver <solver> "
                    "--integrator <integrator> --dt <seconds> --particle-count <n> --steps <n> "
                    "--seed <n>";
            return false;
        }
        if (index + 1 >= argc) {
            error = "missing value for argument: " + arg;
            return false;
        }
        const std::string value(argv[index + 1]);
        if (arg == "--workload") {
            out.workload = value;
        }
        else if (arg == "--solver") {
            out.solver = value;
        }
        else if (arg == "--integrator") {
            out.integrator = value;
        }
        else if (arg == "--dt") {
            if (!parseFloatValue(value, out.dt) || out.dt <= 0.0f) {
                error = "invalid --dt value: " + value;
                return false;
            }
        }
        else if (arg == "--particle-count") {
            if (!parseUintValue(value, out.particleCount) || out.particleCount < 2u) {
                error = "invalid --particle-count value: " + value;
                return false;
            }
        }
        else if (arg == "--steps") {
            if (!parseUintValue(value, out.steps) || out.steps == 0u) {
                error = "invalid --steps value: " + value;
                return false;
            }
        }
        else if (arg == "--seed") {
            if (!parseUintValue(value, out.seed)) {
                error = "invalid --seed value: " + value;
                return false;
            }
        }
        else {
            error = "unknown argument: " + arg;
            return false;
        }
        index += 1;
    }
    if (out.workload.empty()) {
        error = "--workload is required";
        return false;
    }
    return true;
}

/// Description: Describes the run operation contract.
int PerformanceBenchmarkTool::run(int argc, const char* const* argv, std::ostream& out,
                                  std::ostream& err) const
{
    ToolOptions options;
    std::string error;
    if (!parseArgs(argc, argv, options, error)) {
        err << error << "\n";
        return error.rfind("usage:", 0) == 0 ? 0 : 1;
    }
    testsupport::ScenarioConfig cfg;
    if (!grav_test_perf_tool::buildScenario(options, cfg, error)) {
        err << error << "\n";
        return 1;
    }
    testsupport::ScenarioResult result;
    const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    if (!testsupport::runScenario(cfg, result, error)) {
        err << error << "\n";
        return 1;
    }
    const std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - start;
    grav_test_perf_tool::writeMeasurement(options, result, elapsed.count(), out);
    return 0;
}
} // namespace grav_test_perf
