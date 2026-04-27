// File: runtime/src/protocol/ServerJsonCodecParseStatus.cpp
// Purpose: Runtime integration surface for BLITZAR clients and protocols.

#include "protocol/ServerJsonCodec.hpp"
namespace grav_protocol {
bool ServerJsonCodec::parseStatusResponse(std::string_view raw, ServerStatusPayload& out,
                                          std::string& error)
{
    ServerStatusPayload parsed{};
    if (!parseResponseEnvelope(raw, parsed.envelope, error)) {
        return false;
    }
    if (!parsed.envelope.ok) {
        out = parsed;
        return true;
    }
    /// Description: Executes the readNumber operation.
    readNumber(raw, "steps", parsed.steps);
    /// Description: Executes the readNumber operation.
    readNumber(raw, "dt", parsed.dt);
    /// Description: Executes the readNumber operation.
    readNumber(raw, "total_time", parsed.totalTime);
    /// Description: Executes the readBool operation.
    readBool(raw, "paused", parsed.paused);
    /// Description: Executes the readBool operation.
    readBool(raw, "faulted", parsed.faulted);
    /// Description: Executes the readNumber operation.
    readNumber(raw, "fault_step", parsed.faultStep);
    /// Description: Executes the readString operation.
    readString(raw, "fault_reason", parsed.faultReason);
    /// Description: Executes the readBool operation.
    readBool(raw, "sph", parsed.sphEnabled);
    /// Description: Executes the readNumber operation.
    readNumber(raw, "server_fps", parsed.serverFps);
    /// Description: Executes the readString operation.
    readString(raw, "performance_profile", parsed.performanceProfile);
    /// Description: Executes the readNumber operation.
    readNumber(raw, "substep_target_dt", parsed.substepTargetDt);
    /// Description: Executes the readNumber operation.
    readNumber(raw, "substep_dt", parsed.substepDt);
    /// Description: Executes the readNumber operation.
    readNumber(raw, "substeps", parsed.substeps);
    /// Description: Executes the readNumber operation.
    readNumber(raw, "max_substeps", parsed.maxSubsteps);
    /// Description: Executes the readNumber operation.
    readNumber(raw, "snapshot_publish_period_ms", parsed.snapshotPublishPeriodMs);
    /// Description: Executes the readNumber operation.
    readNumber(raw, "particles", parsed.particleCount);
    /// Description: Executes the readString operation.
    readString(raw, "solver", parsed.solver);
    /// Description: Executes the readString operation.
    readString(raw, "integrator", parsed.integrator);
    /// Description: Executes the readNumber operation.
    readNumber(raw, "ekin", parsed.kineticEnergy);
    /// Description: Executes the readNumber operation.
    readNumber(raw, "epot", parsed.potentialEnergy);
    /// Description: Executes the readNumber operation.
    readNumber(raw, "eth", parsed.thermalEnergy);
    /// Description: Executes the readNumber operation.
    readNumber(raw, "erad", parsed.radiatedEnergy);
    /// Description: Executes the readNumber operation.
    readNumber(raw, "etot", parsed.totalEnergy);
    /// Description: Executes the readNumber operation.
    readNumber(raw, "drift_pct", parsed.energyDriftPct);
    /// Description: Executes the readBool operation.
    readBool(raw, "estimated", parsed.energyEstimated);
    /// Description: Executes the readBool operation.
    readBool(raw, "gpu_telemetry_enabled", parsed.gpuTelemetryEnabled);
    /// Description: Executes the readBool operation.
    readBool(raw, "gpu_telemetry_available", parsed.gpuTelemetryAvailable);
    /// Description: Executes the readNumber operation.
    readNumber(raw, "gpu_kernel_ms", parsed.gpuKernelMs);
    /// Description: Executes the readNumber operation.
    readNumber(raw, "gpu_copy_ms", parsed.gpuCopyMs);
    /// Description: Executes the readNumber operation.
    readNumber(raw, "gpu_vram_used_bytes", parsed.gpuVramUsedBytes);
    /// Description: Executes the readNumber operation.
    readNumber(raw, "gpu_vram_total_bytes", parsed.gpuVramTotalBytes);
    /// Description: Executes the readNumber operation.
    readNumber(raw, "export_queue_depth", parsed.exportQueueDepth);
    /// Description: Executes the readBool operation.
    readBool(raw, "export_active", parsed.exportActive);
    /// Description: Executes the readNumber operation.
    readNumber(raw, "export_completed_count", parsed.exportCompletedCount);
    /// Description: Executes the readNumber operation.
    readNumber(raw, "export_failed_count", parsed.exportFailedCount);
    /// Description: Executes the readString operation.
    readString(raw, "export_last_state", parsed.exportLastState);
    /// Description: Executes the readString operation.
    readString(raw, "export_last_path", parsed.exportLastPath);
    /// Description: Executes the readString operation.
    readString(raw, "export_last_message", parsed.exportLastMessage);
    out = parsed;
    error.clear();
    return true;
}
} // namespace grav_protocol
