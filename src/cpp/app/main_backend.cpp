#include "sim/BackendServer.hpp"
#include "sim/SimulationArgs.hpp"
#include "sim/SimulationBackend.hpp"
#include "sim/SimulationConfig.hpp"
#include "sim/SimulationInitConfig.hpp"
#include "sim/TextParse.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace {

std::atomic<bool> g_stopRequested{false};

void onSignal(int)
{
    g_stopRequested.store(true);
}

bool parsePort(std::string_view token, std::uint16_t &out)
{
    unsigned int parsed = 0;
    if (!sim::text::parseNumber(token, parsed) || parsed == 0 || parsed > 65535u) {
        return false;
    }
    out = static_cast<std::uint16_t>(parsed);
    return true;
}

bool parseBackendArgs(
    const std::vector<std::string_view> &rawArgs,
    std::vector<std::string_view> &simArgs,
    std::string &host,
    std::uint16_t &port,
    bool &startPaused,
    bool &showBackendHelp)
{
    simArgs.clear();
    simArgs.reserve(rawArgs.size());
    if (!rawArgs.empty()) {
        simArgs.push_back(rawArgs[0]);
    }

    for (std::size_t i = 1; i < rawArgs.size(); ++i) {
        const std::string token(rawArgs[i]);
        if (token == "--backend-help") {
            showBackendHelp = true;
            continue;
        }
        if (token == "--backend-paused") {
            startPaused = true;
            continue;
        }
        if (token.rfind("--backend-host=", 0) == 0) {
            host = token.substr(std::string("--backend-host=").size());
            continue;
        }
        if (token == "--backend-host") {
            if (i + 1 >= rawArgs.size()) {
                std::cerr << "[backend] missing value after --backend-host\n";
                return false;
            }
            host = std::string(rawArgs[++i]);
            continue;
        }
        if (token.rfind("--backend-port=", 0) == 0) {
            std::uint16_t parsed = 0;
            if (!parsePort(std::string_view(token).substr(std::string("--backend-port=").size()), parsed)) {
                std::cerr << "[backend] invalid --backend-port value\n";
                return false;
            }
            port = parsed;
            continue;
        }
        if (token == "--backend-port") {
            if (i + 1 >= rawArgs.size()) {
                std::cerr << "[backend] missing value after --backend-port\n";
                return false;
            }
            std::uint16_t parsed = 0;
            if (!parsePort(rawArgs[++i], parsed)) {
                std::cerr << "[backend] invalid --backend-port value\n";
                return false;
            }
            port = parsed;
            continue;
        }
        simArgs.push_back(rawArgs[i]);
    }

    return true;
}

void printBackendHelp(std::string_view programName)
{
    std::cout
        << "Backend daemon options:\n"
        << "  --backend-host <ipv4>     Bind address (default: 127.0.0.1)\n"
        << "  --backend-port <1..65535> TCP port (default: 4545)\n"
        << "  --backend-paused          Start simulation paused\n"
        << "  --backend-help            Show this backend-specific help\n"
        << "\n";
    printUsage(std::cout, programName, true);
}

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

} // namespace

int main(int argc, char **argv)
{
    std::vector<std::string_view> args;
    args.reserve(static_cast<std::size_t>(std::max(0, argc)));
    for (int i = 0; i < argc; ++i) {
        args.emplace_back(argv[i] ? argv[i] : "");
    }

    std::string host = "127.0.0.1";
    std::uint16_t port = 4545;
    bool startPaused = false;
    bool showBackendHelp = false;
    std::vector<std::string_view> simArgs;
    if (!parseBackendArgs(args, simArgs, host, port, startPaused, showBackendHelp)) {
        return 2;
    }

    RuntimeArgs runtime;
    runtime.configPath = findConfigPathArg(simArgs, "simulation.ini");
    SimulationBackend backend(runtime.configPath);
    SimulationConfig config = backend.getRuntimeConfig();
    applyArgsToConfig(simArgs, config, runtime, std::cerr);
    if (runtime.hasArgumentError) {
        printBackendHelp(simArgs.empty() ? std::string_view("myAppBackend") : simArgs[0]);
        return 2;
    }
    if (showBackendHelp || runtime.showHelp) {
        printBackendHelp(simArgs.empty() ? std::string_view("myAppBackend") : simArgs[0]);
        return 0;
    }
    if (runtime.saveConfig) {
        config.save(runtime.configPath);
    }

    applyConfigToBackend(backend, config);
    backend.start();
    if (startPaused) {
        backend.setPaused(true);
    }

    BackendServer server(backend);
    if (!server.start(port, host)) {
        std::cerr << "[backend] failed to start IPC server on " << host << ":" << port << "\n";
        backend.stop();
        return 1;
    }

    std::signal(SIGINT, onSignal);
    std::signal(SIGTERM, onSignal);

    std::cout << "[backend] daemon running on " << host << ":" << port
              << " (json control over TCP, newline-delimited)\n";
    std::cout << "[backend] example request: {\"cmd\":\"status\"}\n";

    auto lastLog = std::chrono::steady_clock::now();
    while (!g_stopRequested.load() && !server.shutdownRequested()) {
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
