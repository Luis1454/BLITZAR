/*
 * @file modules/qt/ui/MainWindowPresenter.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Qt desktop user interface module for simulation control and visualization.
 */

#include "ui/MainWindowPresenter.hpp"
#include "types/SimulationTypes.hpp"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <sstream>

namespace bltzr_qt {
class MainWindowPresenterLocal final {
public:
    static bool hasAge(std::uint32_t ageMs)
    {
        return ageMs != std::numeric_limits<std::uint32_t>::max();
    }

    static std::string ageLabel(std::uint32_t ageMs)
    {
        return hasAge(ageMs) ? std::to_string(ageMs) + "ms" : "n/a";
    }

    static std::string latencyLabel(const MainWindowPresentationInput& input)
    {
        if (input.snapshotLatencyMs != std::numeric_limits<std::uint32_t>::max()) {
            return std::to_string(input.snapshotLatencyMs) + "ms";
        }
        if (input.snapshotPipeline.latencyMs != std::numeric_limits<std::uint32_t>::max()) {
            return std::to_string(input.snapshotPipeline.latencyMs) + "ms";
        }
        return "n/a";
    }

    static bool isStale(std::uint32_t ageMs)
    {
        return hasAge(ageMs) && ageMs > 1000u;
    }

    static std::string fixedLabel(float value, int precision)
    {
        std::ostringstream stream;
        stream << std::fixed << std::setprecision(precision) << value;
        return stream.str();
    }

    static std::string bytesLabel(std::uint64_t bytes)
    {
        constexpr double kMiB = 1024.0 * 1024.0;
        std::ostringstream stream;
        stream << std::fixed << std::setprecision(1) << (static_cast<double>(bytes) / kMiB)
               << " MiB";
        return stream.str();
    }

    static float simulatedSecondsPerSecond(const MainWindowPresentationInput& input)
    {
        float simulatedStep = std::max(0.0f, input.stats.dt);
        if (input.stats.substeps > 0u && input.stats.substepDt > 0.0f)
            simulatedStep = input.stats.substepDt * static_cast<float>(input.stats.substeps);
        return std::max(0.0f, simulatedStep * std::max(0.0f, input.stats.serverFps));
    }

    static std::string backendStateLabel(const MainWindowPresentationInput& input, bool staleStats,
                                         bool staleSnapshot)
    {
        if (input.stats.faulted)
            return "faulted";
        if (input.linkLabel != "connected")
            return input.linkLabel;
        if (input.stats.paused)
            return "idle";
        if (staleStats && staleSnapshot)
            return "stalled";
        if (input.stats.serverFps > 0.01f)
            return "busy";
        return "idle";
    }

    static std::string viewportStateLabel(std::uint32_t snapshotAgeMs)
    {
        if (!hasAge(snapshotAgeMs))
            return "awaiting snapshot";
        const std::string age = ageLabel(snapshotAgeMs);
        return isStale(snapshotAgeMs) ? "stale (" + age + " old)" : "fresh (" + age + " old)";
    }

    static std::string progressLabel(const MainWindowPresentationInput& input)
    {
        if (input.simulationHorizonSeconds <= 0.0f)
            return "open-ended";
        const float clampedTime =
            std::clamp(input.stats.totalTime, 0.0f, input.simulationHorizonSeconds);
        const float pct = (clampedTime / input.simulationHorizonSeconds) * 100.0f;
        return fixedLabel(pct, 1) + "% (" + fixedLabel(clampedTime, 2) + " / " +
               fixedLabel(input.simulationHorizonSeconds, 2) + " s)";
    }

    static std::string etaLabel(const MainWindowPresentationInput& input)
    {
        if (input.simulationHorizonSeconds <= 0.0f ||
            input.stats.totalTime >= input.simulationHorizonSeconds)
            return input.simulationHorizonSeconds > 0.0f ? "0.0s" : "n/a";
        const float simRate = simulatedSecondsPerSecond(input);
        if (simRate <= 1e-6f || input.stats.paused || input.stats.faulted ||
            input.linkLabel != "connected")
            return "n/a";
        return fixedLabel((input.simulationHorizonSeconds - input.stats.totalTime) / simRate, 1) +
               "s";
    }

    static std::string exportStateLabel(const MainWindowPresentationInput& input)
    {
        if (input.stats.exportLastState.empty()) {
            return input.stats.exportActive ? "writing" : "idle";
        }
        return input.stats.exportLastState;
    }
};

MainWindowPresentation MainWindowPresenter::present(const MainWindowPresentationInput& input) const
{
    const bool staleStats = MainWindowPresenterLocal::isStale(input.statsAgeMs);
    const bool staleSnapshot = MainWindowPresenterLocal::isStale(input.snapshotAgeMs);
    const std::string faultSuffix =
        input.stats.faulted ? (" [fault@" + std::to_string(input.stats.faultStep) + " " +
                               input.stats.faultReason + "]")
                            : (input.stats.energyEstimated ? " (estimated)" : std::string());
    const std::string latency = MainWindowPresenterLocal::latencyLabel(input);
    const std::string backendState =
        MainWindowPresenterLocal::backendStateLabel(input, staleStats, staleSnapshot);
    const std::string viewportState =
        MainWindowPresenterLocal::viewportStateLabel(input.snapshotAgeMs);
    const float simRate = MainWindowPresenterLocal::simulatedSecondsPerSecond(input);
    const std::string progress = MainWindowPresenterLocal::progressLabel(input);
    const std::string eta = MainWindowPresenterLocal::etaLabel(input);
    const std::string exportState = MainWindowPresenterLocal::exportStateLabel(input);
    MainWindowPresentation output;
    std::ostringstream headline;
    headline << "State: "
             << (input.stats.faulted ? "FAULT" : (input.stats.paused ? "PAUSED" : "RUNNING"))
             << "\nBackend: " << backendState << "\nViewport: " << viewportState
             << "\nLink: " << input.linkLabel << "\nOwner: " << input.ownerLabel
             << "\nEngine: " << input.stats.solverName << " / " << input.stats.integratorName
             << "\nProfile: " << input.performanceProfile
             << "\nSPH: " << (input.stats.sphEnabled ? "on" : "off");
    if (!faultSuffix.empty()) {
        headline << "\nCondition: " << faultSuffix.substr(1);
    }
    output.headlineText = headline.str();
    std::ostringstream runtime;
    runtime << "dt: " << input.stats.dt << " s"
            << "\nSubsteps: " << input.stats.substeps << " x " << input.stats.substepDt << " s"
            << "\nTarget step: " << input.stats.substepTargetDt << " s"
            << "\nSubstep limit: " << input.stats.maxSubsteps
            << "\nServer rate: " << input.stats.serverFps << " step/s"
            << "\nSim rate: " << MainWindowPresenterLocal::fixedLabel(simRate, 2) << " sim s/s"
            << "\nPublish cadence: " << input.stats.snapshotPublishPeriodMs << " ms"
            << "\nUI rate: " << input.uiTickFps << " fps";
    output.runtimeText = runtime.str();
    std::ostringstream queue;
    queue << "Progress: " << progress << "\nETA: " << eta << "\nExport: " << exportState
          << "\nExport backlog: " << input.stats.exportQueueDepth
          << "\nExport done: " << input.stats.exportCompletedCount
          << "\nExport failed: " << input.stats.exportFailedCount << "\nLast export: "
          << (input.stats.exportLastPath.empty() ? "n/a" : input.stats.exportLastPath)
          << "\nSteps: " << input.stats.steps << "\nParticles: " << input.stats.particleCount
          << "\nDraw budget: " << input.displayedParticles << " / " << input.clientDrawCap
          << "\nStats age: " << MainWindowPresenterLocal::ageLabel(input.statsAgeMs)
          << "\nSnapshot age: " << MainWindowPresenterLocal::ageLabel(input.snapshotAgeMs)
          << "\nQueue: " << input.snapshotPipeline.queueDepth << " / "
          << input.snapshotPipeline.queueCapacity
          << "\nDropped: " << input.snapshotPipeline.droppedFrames << "\nLatency: " << latency
          << "\nPolicy: " << input.snapshotPipeline.dropPolicy;
    output.queueText = queue.str();
    std::ostringstream energy;
    energy << "Total: " << input.stats.totalEnergy << " J"
           << "\nDrift: " << input.stats.energyDriftPct << "%"
           << "\nKinetic: " << input.stats.kineticEnergy << " J"
           << "\nPotential: " << input.stats.potentialEnergy << " J"
           << "\nThermal: " << input.stats.thermalEnergy << " J"
           << "\nRadiated: " << input.stats.radiatedEnergy << " J";
    output.energyText = energy.str();
    std::ostringstream gpu;
    if (!input.stats.gpuTelemetryEnabled) {
        gpu << "State: off"
            << "\nSampling: disabled";
    }
    else if (!input.stats.gpuTelemetryAvailable) {
        gpu << "State: waiting"
            << "\nSampling: every 8 steps";
    }
    else {
        gpu << "State: active"
            << "\nSampling: every 8 steps"
            << "\nKernel: " << MainWindowPresenterLocal::fixedLabel(input.stats.gpuKernelMs, 3)
            << " ms"
            << "\nCopy: " << MainWindowPresenterLocal::fixedLabel(input.stats.gpuCopyMs, 3) << " ms"
            << "\nVRAM: " << MainWindowPresenterLocal::bytesLabel(input.stats.gpuVramUsedBytes)
            << " / " << MainWindowPresenterLocal::bytesLabel(input.stats.gpuVramTotalBytes);
    }
    output.gpuText = gpu.str();
    output.statusText = output.headlineText + "\n" + output.runtimeText + "\n" + output.queueText +
                        "\n" + output.energyText + "\n" + output.gpuText;
    std::ostringstream trace;
    trace << "[qt] step=" << input.stats.steps << " link=" << input.linkLabel
          << " owner=" << input.ownerLabel << " solver=" << input.stats.solverName
          << " integrator=" << input.stats.integratorName << " perf=" << input.performanceProfile
          << " sph=" << (input.stats.sphEnabled ? "on" : "off")
          << " step_s=" << input.stats.serverFps << " substeps=" << input.stats.substeps
          << " substep_dt=" << input.stats.substepDt
          << " substep_target_dt=" << input.stats.substepTargetDt
          << " max_substeps=" << input.stats.maxSubsteps
          << " snapshot_publish_ms=" << input.stats.snapshotPublishPeriodMs
          << " backend_state=" << backendState << " viewport_state=\"" << viewportState << "\""
          << " sim_rate=" << MainWindowPresenterLocal::fixedLabel(simRate, 2) << " progress=\""
          << progress << "\""
          << " eta=\"" << eta << "\""
          << " export_state=" << exportState << " export_backlog=" << input.stats.exportQueueDepth
          << " export_done=" << input.stats.exportCompletedCount
          << " export_failed=" << input.stats.exportFailedCount << " export_path=\""
          << input.stats.exportLastPath << "\""
          << " export_message=\"" << input.stats.exportLastMessage << "\""
          << " ui_fps=" << input.uiTickFps << " draw=" << input.displayedParticles
          << " draw_cap=" << input.clientDrawCap
          << " stats_age_ms=" << MainWindowPresenterLocal::ageLabel(input.statsAgeMs)
          << " snap_age_ms=" << MainWindowPresenterLocal::ageLabel(input.snapshotAgeMs)
          << " queue_depth=" << input.snapshotPipeline.queueDepth
          << " queue_capacity=" << input.snapshotPipeline.queueCapacity
          << " dropped_frames=" << input.snapshotPipeline.droppedFrames
          << " snapshot_latency_ms=" << latency
          << " drop_policy=" << input.snapshotPipeline.dropPolicy << " stale="
          << ((input.linkLabel != "connected" || staleStats || staleSnapshot) ? "1" : "0")
          << " faulted=" << (input.stats.faulted ? "1" : "0")
          << " fault_step=" << input.stats.faultStep << " fault_reason=\""
          << input.stats.faultReason << "\""
          << " energy=" << input.stats.totalEnergy << " thermal=" << input.stats.thermalEnergy
          << " radiated=" << input.stats.radiatedEnergy
          << " drift_pct=" << input.stats.energyDriftPct
          << " gpu_telemetry=" << (input.stats.gpuTelemetryEnabled ? "on" : "off")
          << " gpu_available=" << (input.stats.gpuTelemetryAvailable ? "1" : "0")
          << " gpu_kernel_ms=" << MainWindowPresenterLocal::fixedLabel(input.stats.gpuKernelMs, 3)
          << " gpu_copy_ms=" << MainWindowPresenterLocal::fixedLabel(input.stats.gpuCopyMs, 3)
          << " gpu_vram_used=\""
          << MainWindowPresenterLocal::bytesLabel(input.stats.gpuVramUsedBytes) << "\""
          << " gpu_vram_total=\""
          << MainWindowPresenterLocal::bytesLabel(input.stats.gpuVramTotalBytes) << "\""
          << (input.stats.energyEstimated ? " est" : "");
    output.consoleTrace = trace.str();
    return output;
}
} // namespace bltzr_qt
