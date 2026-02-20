#include "sim/SimulationBackend.hpp"
#include "sim/SimulationArgs.hpp"
#include "sim/SimulationConfig.hpp"
#include "sim/SimulationInitConfig.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace {
int parseIntValue(const std::string &value, int fallback)
{
    char *end = nullptr;
    const long parsed = std::strtol(value.c_str(), &end, 10);
    if (end == value.c_str() || parsed <= 0) {
        return fallback;
    }
    return static_cast<int>(parsed);
}

float parseFloatValue(const std::string &value, float fallback)
{
    char *end = nullptr;
    const float parsed = std::strtof(value.c_str(), &end);
    if (end == value.c_str() || parsed <= 0.0f) {
        return fallback;
    }
    return parsed;
}

bool parseBoolEnv(const char *name, bool fallback)
{
    if (const char *value = std::getenv(name)) {
        if (std::string(value) == "1" || std::string(value) == "true" || std::string(value) == "on") {
            return true;
        }
        if (std::string(value) == "0" || std::string(value) == "false" || std::string(value) == "off") {
            return false;
        }
    }
    return fallback;
}
}

int main(int argc, char **argv)
{
    RuntimeArgs runtime;
    runtime.configPath = findConfigPathArg(argc, argv, "simulation.ini");
    SimulationConfig config = SimulationConfig::loadOrCreate(runtime.configPath);
    applyArgsToConfig(argc, argv, config, runtime, std::cerr);
    if (runtime.showHelp) {
        printUsage(std::cout, argv[0] ? argv[0] : "myApp.exe", true);
        return 0;
    }
    if (runtime.saveConfig) {
        config.save(runtime.configPath);
    }

    int particleCount = static_cast<int>(config.particleCount);
    int targetSteps = runtime.targetSteps;
    float dt = config.dt;
    std::string solver = config.solver;
    std::string exportFormat = config.exportFormat;
    std::string integrator = config.integrator;

    // Backward compatibility: positional args still work.
    if (!runtime.positional.empty()) {
        particleCount = parseIntValue(runtime.positional[0], particleCount);
    }
    if (runtime.positional.size() > 1) {
        targetSteps = parseIntValue(runtime.positional[1], targetSteps);
    }
    if (runtime.positional.size() > 2) {
        dt = parseFloatValue(runtime.positional[2], dt);
    }
    if (runtime.positional.size() > 3 && !runtime.positional[3].empty()) {
        solver = runtime.positional[3];
    }
    if (runtime.positional.size() > 4 && !runtime.positional[4].empty()) {
        exportFormat = runtime.positional[4];
    }
    if (runtime.positional.size() > 5 && !runtime.positional[5].empty()) {
        integrator = runtime.positional[5];
    }

    const bool exportOnExit = parseBoolEnv("GRAVITY_EXPORT_ON_EXIT", runtime.exportOnExit);

    std::cout << "[headless] start particles=" << particleCount
              << " targetSteps=" << targetSteps
              << " dt=" << dt
              << " solver=" << solver
              << " integrator=" << integrator
              << " sph=" << (config.sphEnabled ? "on" : "off")
              << " export=" << exportFormat
              << "\n";
    if (!config.inputFile.empty()) {
        std::cout << "[headless] input_file=" << config.inputFile
                  << " input_format=" << config.inputFormat << "\n";
    }

    SimulationBackend backend(static_cast<std::uint32_t>(particleCount), dt);
    backend.setSolverMode(solver);
    backend.setIntegratorMode(integrator);
    backend.setOctreeParameters(config.octreeTheta, config.octreeSoftening);
    backend.setSphEnabled(config.sphEnabled);
    backend.setSphParameters(
        config.sphSmoothingLength,
        config.sphRestDensity,
        config.sphGasConstant,
        config.sphViscosity
    );
    backend.setEnergyMeasurementConfig(config.energyMeasureEverySteps, config.energySampleLimit);
    backend.setExportDefaults(config.exportDirectory, exportFormat);
    backend.setInitialStateFile(config.inputFile, config.inputFormat);
    backend.setInitialStateConfig(buildInitialStateConfig(config));
    backend.start();

    std::vector<RenderParticle> snapshot;
    const auto start = std::chrono::steady_clock::now();
    std::uint64_t lastLoggedStep = 0;

    while (true) {
        const SimulationStats stats = backend.getStats();
        if (stats.steps >= static_cast<std::uint64_t>(targetSteps)) {
            break;
        }

        if (backend.tryConsumeSnapshot(snapshot) && !snapshot.empty()) {
            if ((stats.steps / 50) > (lastLoggedStep / 50)) {
                lastLoggedStep = stats.steps;
                const RenderParticle &center = snapshot[0];
                std::cout << "step=" << stats.steps
                          << " solver=" << stats.solverName
                          << " center=(" << center.x << ", " << center.y << ", " << center.z << ")"
                          << " snapshot=" << snapshot.size()
                          << " backend_step_s=" << stats.backendFps
                          << " energy=" << stats.totalEnergy
                          << " Eth=" << stats.thermalEnergy
                          << " Erad=" << stats.radiatedEnergy
                          << " drift_pct=" << stats.energyDriftPct
                          << (stats.energyEstimated ? " est" : "")
                          << "\n";
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    if (exportOnExit) {
        backend.setPaused(true);
        backend.requestExportSnapshot("", exportFormat);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    backend.stop();

    const auto end = std::chrono::steady_clock::now();
    const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    const SimulationStats finalStats = backend.getStats();

    std::cout << "[headless] done particles=" << finalStats.particleCount
              << " steps=" << finalStats.steps
              << " dt=" << dt
              << " solver=" << finalStats.solverName
              << " time_ms=" << elapsedMs
              << " Ekin=" << finalStats.kineticEnergy
              << " Epot=" << finalStats.potentialEnergy
              << " Eth=" << finalStats.thermalEnergy
              << " Erad=" << finalStats.radiatedEnergy
              << " Etot=" << finalStats.totalEnergy
              << " drift_pct=" << finalStats.energyDriftPct
              << (finalStats.energyEstimated ? " est" : "")
              << "\n";

    return 0;
}

