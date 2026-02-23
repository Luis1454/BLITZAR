#include "sim/SimulationBackend.hpp"
#include "sim/SimulationArgs.hpp"
#include "sim/SimulationConfig.hpp"
#include "sim/EnvUtils.hpp"
#include "sim/SimulationInitConfig.hpp"

#include <chrono>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

int main(int argc, char **argv)
{
    std::vector<std::string_view> args;
    args.reserve(static_cast<std::size_t>(std::max(0, argc)));
    for (int i = 0; i < argc; ++i) {
        args.emplace_back(argv[i] ? argv[i] : "");
    }

    RuntimeArgs runtime;
    runtime.configPath = findConfigPathArg(args, "simulation.ini");
    SimulationBackend backend(runtime.configPath);
    SimulationConfig config = backend.getRuntimeConfig();
    applyArgsToConfig(args, config, runtime, std::cerr);
    if (runtime.hasArgumentError) {
        printUsage(std::cerr, args.empty() ? std::string_view("myAppHeadless") : args[0], true);
        return 2;
    }
    if (runtime.showHelp) {
        printUsage(std::cout, args.empty() ? std::string_view("myApp.exe") : args[0], true);
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

    const bool exportOnExit = sim::env::getBool("GRAVITY_EXPORT_ON_EXIT", runtime.exportOnExit);

    std::cout << "[headless] start particles=" << particleCount
              << " targetSteps=" << targetSteps
              << " dt=" << dt
              << " solver=" << solver
              << " integrator=" << integrator
              << " sph=" << (config.sphEnabled ? "on" : "off")
              << " export=" << exportFormat
              << " export_on_exit=" << (exportOnExit ? "on" : "off")
              << "\n";
    if (!config.inputFile.empty()) {
        std::cout << "[headless] input_file=" << config.inputFile
                  << " input_format=" << config.inputFormat << "\n";
    }

    backend.setParticleCount(static_cast<std::uint32_t>(particleCount));
    backend.setDt(dt);
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
                          << " faulted=" << (stats.faulted ? "1" : "0")
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
              << " faulted=" << (finalStats.faulted ? "1" : "0")
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
