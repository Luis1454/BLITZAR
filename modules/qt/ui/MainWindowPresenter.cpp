#include "ui/MainWindowPresenter.hpp"

#include "types/SimulationTypes.hpp"

#include <chrono>
#include <cstdint>
#include <limits>
#include <sstream>

namespace grav_qt {

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

        static std::string latencyLabel(const MainWindowPresentationInput &input)
        {
            if (input.snapshotLatencyMs != std::numeric_limits<std::uint32_t>::max()) {
                return std::to_string(input.snapshotLatencyMs) + "ms";
            }
            if (input.snapshotPipeline.latencyMs != std::numeric_limits<std::uint32_t>::max()) {
                return std::to_string(input.snapshotPipeline.latencyMs) + "ms";
            }
            return "n/a";
        }
};

MainWindowPresentation MainWindowPresenter::present(const MainWindowPresentationInput &input) const
{
    const bool staleStats = MainWindowPresenterLocal::hasAge(input.statsAgeMs) && input.statsAgeMs > 1000u;
    const bool staleSnapshot = MainWindowPresenterLocal::hasAge(input.snapshotAgeMs) && input.snapshotAgeMs > 1000u;
    const bool stale = input.linkLabel != "connected" || staleStats || staleSnapshot;
    const std::string faultSuffix = input.stats.faulted
        ? (" [fault@" + std::to_string(input.stats.faultStep) + " " + input.stats.faultReason + "]")
        : (input.stats.energyEstimated ? " (estimated)" : std::string());
    const std::string latency = MainWindowPresenterLocal::latencyLabel(input);

    MainWindowPresentation output;
    std::ostringstream headline;
    headline << "State: " << (input.stats.faulted ? "FAULT" : (input.stats.paused ? "PAUSED" : "RUNNING"))
             << "\nLink: " << input.linkLabel
             << "\nOwner: " << input.ownerLabel
             << "\nEngine: " << input.stats.solverName << " / " << input.stats.integratorName
             << "\nProfile: " << input.performanceProfile
             << "\nSPH: " << (input.stats.sphEnabled ? "on" : "off");
    if (stale) {
        headline << "\nData freshness: stale";
    }
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
            << "\nUI rate: " << input.uiTickFps << " fps";
    output.runtimeText = runtime.str();

    std::ostringstream queue;
    queue << "Steps: " << input.stats.steps
          << "\nParticles: " << input.stats.particleCount
          << "\nDraw budget: " << input.displayedParticles << " / " << input.clientDrawCap
          << "\nStats age: " << MainWindowPresenterLocal::ageLabel(input.statsAgeMs)
          << "\nSnapshot age: " << MainWindowPresenterLocal::ageLabel(input.snapshotAgeMs)
          << "\nQueue: " << input.snapshotPipeline.queueDepth << " / " << input.snapshotPipeline.queueCapacity
          << "\nDropped: " << input.snapshotPipeline.droppedFrames
          << "\nLatency: " << latency
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
    output.statusText = output.headlineText + "\n" + output.runtimeText + "\n" + output.queueText + "\n" + output.energyText;

    std::ostringstream trace;
    trace << "[qt] step=" << input.stats.steps
          << " link=" << input.linkLabel
          << " owner=" << input.ownerLabel
          << " solver=" << input.stats.solverName
          << " integrator=" << input.stats.integratorName
          << " perf=" << input.performanceProfile
          << " sph=" << (input.stats.sphEnabled ? "on" : "off")
          << " step_s=" << input.stats.serverFps
          << " substeps=" << input.stats.substeps
          << " substep_dt=" << input.stats.substepDt
          << " substep_target_dt=" << input.stats.substepTargetDt
          << " max_substeps=" << input.stats.maxSubsteps
          << " snapshot_publish_ms=" << input.stats.snapshotPublishPeriodMs
          << " ui_fps=" << input.uiTickFps
          << " draw=" << input.displayedParticles
          << " draw_cap=" << input.clientDrawCap
          << " stats_age_ms=" << MainWindowPresenterLocal::ageLabel(input.statsAgeMs)
          << " snap_age_ms=" << MainWindowPresenterLocal::ageLabel(input.snapshotAgeMs)
          << " queue_depth=" << input.snapshotPipeline.queueDepth
          << " queue_capacity=" << input.snapshotPipeline.queueCapacity
          << " dropped_frames=" << input.snapshotPipeline.droppedFrames
          << " snapshot_latency_ms=" << latency
          << " drop_policy=" << input.snapshotPipeline.dropPolicy
          << " stale=" << (stale ? "1" : "0")
          << " faulted=" << (input.stats.faulted ? "1" : "0")
          << " fault_step=" << input.stats.faultStep
          << " fault_reason=\"" << input.stats.faultReason << "\""
          << " energy=" << input.stats.totalEnergy
          << " thermal=" << input.stats.thermalEnergy
          << " radiated=" << input.stats.radiatedEnergy
          << " drift_pct=" << input.stats.energyDriftPct
          << (input.stats.energyEstimated ? " est" : "");
    output.consoleTrace = trace.str();
    return output;
}

} // namespace grav_qt
