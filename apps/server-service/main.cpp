// File: apps/server-service/main.cpp
// Purpose: Application entry point or host support for BLITZAR executables.

#include "apps/server-service/server_args.hpp"
#include "config/SimulationArgs.hpp"
#include "config/SimulationConfig.hpp"
#include "server/ServerDaemon.hpp"
#include "server/SimulationInitConfig.hpp"
#include "server/SimulationServer.hpp"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace grav_server_service {
/// Description: Executes the applyConfigToServer operation.
void applyConfigToServer(SimulationServer& server, const SimulationConfig& config)
{
    const ResolvedInitialStatePlan initPlan = resolveInitialStatePlan(config, std::cerr);
    server.setParticleCount(std::max<std::uint32_t>(2u, config.particleCount));
    server.setDt(config.dt);
    server.setSolverMode(config.solver);
    server.setIntegratorMode(config.integrator);
    server.setOctreeParameters(config.octreeTheta, config.octreeSoftening);
    server.setSphEnabled(config.sphEnabled);
    server.setSphParameters(config.sphSmoothingLength, config.sphRestDensity, config.sphGasConstant,
                            config.sphViscosity);
    server.setEnergyMeasurementConfig(config.energyMeasureEverySteps, config.energySampleLimit);
    server.setExportDefaults(config.exportDirectory, config.exportFormat);
    server.setInitialStateFile(initPlan.inputFile, initPlan.inputFormat);
    server.setInitialStateConfig(initPlan.config);
    std::cout << "[server] " << initPlan.summary << "\n";
}
} // namespace grav_server_service

/// Description: Executes the main operation.
int main(int argc, char** argv)
{
    std::vector<std::string_view> args;
    args.reserve(static_cast<std::size_t>(std::max(0, argc)));
    for (int i = 0; i < argc; ++i)
        args.emplace_back(argv[i] ? argv[i] : "");
    grav_server_service::resetStopRequested();
    grav_server_service::DaemonOptions options{};
    if (!grav_server_service::parseServerArgs(args, options, std::cerr)) {
        return 2;
    }
    RuntimeArgs runtime;
    runtime.configPath = findConfigPathArg(options.simArgs, "simulation.ini");
    SimulationServer simulationServer(runtime.configPath);
    SimulationConfig config = simulationServer.getRuntimeConfig();
    applyArgsToConfig(options.simArgs, config, runtime, std::cerr);
    if (runtime.hasArgumentError) {
        grav_server_service::printServerHelp(
            options.simArgs.empty() ? std::string_view("blitzar-server") : options.simArgs[0]);
        return 2;
    }
    if (options.showServerHelp || runtime.showHelp) {
        grav_server_service::printServerHelp(
            options.simArgs.empty() ? std::string_view("blitzar-server") : options.simArgs[0]);
        return 0;
    }
    if (runtime.saveConfig) {
        config.save(runtime.configPath);
    }
    if (!grav_server_service::isLoopbackBindHost(options.host) && !options.allowRemoteBind) {
        std::cerr << "[server] refusing non-loopback bind host '" << options.host
                  << "' without --server-allow-remote\n";
        return 2;
    }
    grav_server_service::applyConfigToServer(simulationServer, config);
    simulationServer.start();
    if (options.startPaused) {
        simulationServer.setPaused(true);
    }
    ServerDaemon daemon(simulationServer, options.authToken);
    if (!daemon.start(options.port, options.host)) {
        std::cerr << "[server] failed to start IPC server on " << options.host << ":"
                  << options.port << "\n";
        simulationServer.stop();
        return 1;
    }
    grav_server_service::installStopSignalHandlers();
    std::cout << "[server] daemon running on " << options.host << ":" << options.port
              << " (json control over TCP, newline-delimited)\n";
    std::cout << "[server] example request: {\"cmd\":\"status\"}\n";
    auto lastLog = std::chrono::steady_clock::now();
    while (!grav_server_service::stopRequested() && !daemon.shutdownRequested()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        const auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - lastLog).count() >= 2) {
            lastLog = now;
            const SimulationStats stats = simulationServer.getStats();
            std::cout << "[server] step=" << stats.steps << " paused=" << (stats.paused ? "1" : "0")
                      << " faulted=" << (stats.faulted ? "1" : "0") << " dt=" << stats.dt
                      << " solver=" << stats.solverName << " sps=" << stats.serverFps
                      << " particles=" << stats.particleCount << "\n";
        }
    }
    std::cout << "[server] stopping...\n";
    daemon.stop();
    simulationServer.stop();
    return 0;
}
