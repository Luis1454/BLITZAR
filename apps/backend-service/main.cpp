#include "backend/BackendServer.hpp"
#include "config/SimulationArgs.hpp"
#include "backend/SimulationBackend.hpp"
#include "config/SimulationConfig.hpp"
#include "backend/SimulationInitConfig.hpp"
#include "apps/backend-service/backend_args.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace grav_backend_service {

void applyConfigToBackend(SimulationBackend &backend, const SimulationConfig &config)
{
    backend.setParticleCount(std::max<std::uint32_t>(2u, config.particleCount));
    backend.setDt(config.dt);
    backend.setSolverMode(config.solver);
    backend.setIntegratorMode(config.integrator);
    backend.setOctreeParameters(config.octreeTheta, config.octreeSoftening);
    backend.setSphEnabled(config.sphEnabled);
    backend.setSphParameters(
        config.sphSmoothingLength,
        config.sphRestDensity,
        config.sphGasConstant,
        config.sphViscosity
    );
    backend.setEnergyMeasurementConfig(config.energyMeasureEverySteps, config.energySampleLimit);
    backend.setExportDefaults(config.exportDirectory, config.exportFormat);
    backend.setInitialStateFile(config.inputFile, config.inputFormat);
    backend.setInitialStateConfig(buildInitialStateConfig(config));
}
} // namespace grav_backend_service
int main(int argc, char **argv)
{
    std::vector<std::string_view> args;
    args.reserve(static_cast<std::size_t>(std::max(0, argc)));
    for (int i = 0; i < argc; ++i) {
        args.emplace_back(argv[i] ? argv[i] : "");
    }

    grav_backend_service::resetStopRequested();
    grav_backend_service::DaemonOptions options{};
    if (!grav_backend_service::parseBackendArgs(args, options, std::cerr)) {
        return 2;
    }

    RuntimeArgs runtime;
    runtime.configPath = findConfigPathArg(options.simArgs, "simulation.ini");
    SimulationBackend backend(runtime.configPath);
    SimulationConfig config = backend.getRuntimeConfig();
    applyArgsToConfig(options.simArgs, config, runtime, std::cerr);
    if (runtime.hasArgumentError) {
        grav_backend_service::printBackendHelp(options.simArgs.empty() ? std::string_view("myAppBackend") : options.simArgs[0]);
        return 2;
    }
    if (options.showBackendHelp || runtime.showHelp) {
        grav_backend_service::printBackendHelp(options.simArgs.empty() ? std::string_view("myAppBackend") : options.simArgs[0]);
        return 0;
    }
    if (runtime.saveConfig) {
        config.save(runtime.configPath);
    }

    if (!grav_backend_service::isLoopbackBindHost(options.host) && !options.allowRemoteBind) {
        std::cerr << "[backend] refusing non-loopback bind host '" << options.host
                  << "' without --backend-allow-remote\n";
        return 2;
    }

    grav_backend_service::applyConfigToBackend(backend, config);
    backend.start();
    if (options.startPaused) {
        backend.setPaused(true);
    }

    BackendServer server(backend, options.authToken);
    if (!server.start(options.port, options.host)) {
        std::cerr << "[backend] failed to start IPC server on " << options.host << ":" << options.port << "\n";
        backend.stop();
        return 1;
    }

    grav_backend_service::installStopSignalHandlers();

    std::cout << "[backend] daemon running on " << options.host << ":" << options.port
              << " (json control over TCP, newline-delimited)\n";
    std::cout << "[backend] example request: {\"cmd\":\"status\"}\n";

    auto lastLog = std::chrono::steady_clock::now();
    while (!grav_backend_service::stopRequested() && !server.shutdownRequested()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        const auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - lastLog).count() >= 2) {
            lastLog = now;
            const SimulationStats stats = backend.getStats();
            std::cout << "[backend] step=" << stats.steps
                      << " paused=" << (stats.paused ? "1" : "0")
                      << " faulted=" << (stats.faulted ? "1" : "0")
                      << " dt=" << stats.dt
                      << " solver=" << stats.solverName
                      << " sps=" << stats.backendFps
                      << " particles=" << stats.particleCount
                      << "\n";
        }
    }

    std::cout << "[backend] stopping...\n";
    server.stop();
    backend.stop();
    return 0;
}
