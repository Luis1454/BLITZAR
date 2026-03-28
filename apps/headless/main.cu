#include "config/EnvUtils.hpp"
#include "config/SimulationArgs.hpp"
#include "config/SimulationConfig.hpp"
#include "server/SimulationInitConfig.hpp"
#include "server/SimulationServer.hpp"
#include <chrono>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>
int main(int argc, char** argv)
{
    std::vector<std::string_view> args;
    args.reserve(static_cast<std::size_t>(std::max(0, argc)));
    for (int i = 0; i < argc; ++i)
        args.emplace_back(argv[i] ? argv[i] : "");
    RuntimeArgs runtime;
    runtime.configPath = findConfigPathArg(args, "simulation.ini");
    SimulationServer server(runtime.configPath);
    SimulationConfig config = server.getRuntimeConfig();
    applyArgsToConfig(args, config, runtime, std::cerr);
    if (runtime.hasArgumentError) {
        printUsage(std::cerr, args.empty() ? std::string_view("blitzar-headless") : args[0], true);
        return 2;
    }
    if (runtime.showHelp) {
        printUsage(std::cout, args.empty() ? std::string_view("blitzar.exe") : args[0], true);
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
    const ResolvedInitialStatePlan initPlan = resolveInitialStatePlan(config, std::cerr);
    bool exportOnExit = runtime.exportOnExit;
    constexpr bool kDevProfile = GRAVITY_PROFILE_IS_DEV != 0;
    if (kDevProfile)
        exportOnExit = grav_env::getBool("GRAVITY_EXPORT_ON_EXIT", runtime.exportOnExit);
    std::cout << "[headless] start particles=" << particleCount << " targetSteps=" << targetSteps
              << " dt=" << dt << " solver=" << solver << " integrator=" << integrator
              << " sph=" << (config.sphEnabled ? "on" : "off") << " export=" << exportFormat
              << " export_on_exit=" << (exportOnExit ? "on" : "off") << "\n";
    std::cout << "[headless] " << initPlan.summary << "\n";
    server.setParticleCount(static_cast<std::uint32_t>(particleCount));
    server.setDt(dt);
    server.setSolverMode(solver);
    server.setIntegratorMode(integrator);
    server.setOctreeParameters(config.octreeTheta, config.octreeSoftening);
    server.setSphEnabled(config.sphEnabled);
    server.setSphParameters(config.sphSmoothingLength, config.sphRestDensity, config.sphGasConstant,
                            config.sphViscosity);
    server.setEnergyMeasurementConfig(config.energyMeasureEverySteps, config.energySampleLimit);
    server.setExportDefaults(config.exportDirectory, exportFormat);
    server.setInitialStateFile(initPlan.inputFile, initPlan.inputFormat);
    server.setInitialStateConfig(initPlan.config);
    server.start();
    std::vector<RenderParticle> snapshot;
    const auto start = std::chrono::steady_clock::now();
    std::uint64_t lastLoggedStep = 0;
    SimulationStats stats = server.getStats();
    while (stats.steps < static_cast<std::uint64_t>(targetSteps)) {
        if (server.tryConsumeSnapshot(snapshot) && !snapshot.empty()) {
            if ((stats.steps / 50) > (lastLoggedStep / 50)) {
                lastLoggedStep = stats.steps;
                const RenderParticle& center = snapshot[0];
                std::cout << "step=" << stats.steps << " solver=" << stats.solverName
                          << " faulted=" << (stats.faulted ? "1" : "0") << " center=(" << center.x
                          << ", " << center.y << ", " << center.z << ")"
                          << " snapshot=" << snapshot.size() << " server_step_s=" << stats.serverFps
                          << " energy=" << stats.totalEnergy << " Eth=" << stats.thermalEnergy
                          << " Erad=" << stats.radiatedEnergy
                          << " drift_pct=" << stats.energyDriftPct
                          << (stats.energyEstimated ? " est" : "") << "\n";
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        stats = server.getStats();
    }
    if (exportOnExit) {
        server.setPaused(true);
        server.requestExportSnapshot("", exportFormat);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    server.stop();
    const auto end = std::chrono::steady_clock::now();
    const auto elapsedMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    const SimulationStats finalStats = server.getStats();
    std::cout << "[headless] done particles=" << finalStats.particleCount
              << " steps=" << finalStats.steps << " faulted=" << (finalStats.faulted ? "1" : "0")
              << " dt=" << dt << " solver=" << finalStats.solverName << " time_ms=" << elapsedMs
              << " Ekin=" << finalStats.kineticEnergy << " Epot=" << finalStats.potentialEnergy
              << " Eth=" << finalStats.thermalEnergy << " Erad=" << finalStats.radiatedEnergy
              << " Etot=" << finalStats.totalEnergy << " drift_pct=" << finalStats.energyDriftPct
              << (finalStats.energyEstimated ? " est" : "") << "\n";
    return 0;
}
