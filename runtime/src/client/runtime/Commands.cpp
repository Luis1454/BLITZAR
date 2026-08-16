/*
 * @file runtime/src/client/runtime/Commands.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Simulation command serialization and dispatch for the client facade.
 */

#include "Constants.hpp"
#include "client/runtime/Bridge.hpp"
#include "protocol/Protocol.hpp"
#include <algorithm>

namespace bltzr_client {

void Bridge::setPaused(bool paused)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    sendOrQueueRemote(std::string(paused ? bltzr_protocol::Pause : bltzr_protocol::Resume));
}

void Bridge::togglePaused()
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    sendOrQueueRemote(std::string(bltzr_protocol::Toggle));
}

void Bridge::stepOnce()
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    sendOrQueueRemote(std::string(bltzr_protocol::Step), "\"count\":1");
}

void Bridge::setParticleCount(std::uint32_t particleCount)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    const std::uint32_t clamped = std::max<std::uint32_t>(2u, particleCount);
    sendOrQueueRemote(std::string(bltzr_protocol::SetParticleCount),
                      "\"value\":" + std::to_string(clamped));
}

void Bridge::setDt(float dt)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    const float clamped = std::max(1e-6f, dt);
    sendOrQueueRemote(std::string(bltzr_protocol::SetDt), "\"value\":" + std::to_string(clamped));
}

void Bridge::scaleDt(float factor)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    const float currentDt = std::max(1e-6f, getStats().dt);
    setDt(std::max(1e-6f, currentDt * factor));
}

void Bridge::requestReset()
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    sendOrQueueRemote(std::string(bltzr_protocol::Reset));
}

void Bridge::requestRecover()
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    sendOrQueueRemote(std::string(bltzr_protocol::Recover));
}

void Bridge::setSolverMode(const std::string& mode)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    sendOrQueueRemote(std::string(bltzr_protocol::SetSolver),
                      "\"value\":\"" + jsonEscape(mode) + "\"");
}

void Bridge::setIntegratorMode(const std::string& mode)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    sendOrQueueRemote(std::string(bltzr_protocol::SetIntegrator),
                      "\"value\":\"" + jsonEscape(mode) + "\"");
}

void Bridge::setPerformanceProfile(const std::string& profile)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    sendOrQueueRemote(std::string(bltzr_protocol::SetPerformanceProfile),
                      "\"value\":\"" + jsonEscape(profile) + "\"");
}

void Bridge::setTreePmAssignment(const std::string& assignment)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    sendOrQueueRemote(std::string(bltzr_protocol::SetTreePmAssignment),
                      "\"value\":\"" + jsonEscape(assignment) + "\"");
}

void Bridge::setTreePmParameters(bool enabled, const std::string& model, const std::string& layout,
                                 const std::string& precision, const std::string& assignment,
                                 bool localGrid, std::uint32_t gridSize,
                                 std::uint32_t jacobiIterations, float cutoffFactor,
                                 std::uint32_t maxLocalNeighbors, std::uint32_t particleLimit,
                                 std::uint32_t denseCellThreshold, bool gravityOnlyBuffers)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    sendOrQueueRemote(
        std::string(bltzr_protocol::SetTreePmParameters),
        std::string("\"enabled\":") + (enabled ? "true" : "false") + ",\"model\":\"" +
            jsonEscape(model) + "\",\"layout\":\"" + jsonEscape(layout) + "\",\"precision\":\"" +
            jsonEscape(precision) + "\",\"assignment\":\"" + jsonEscape(assignment) +
            "\",\"local_grid\":" + (localGrid ? "true" : "false") +
            ",\"grid_size\":" + std::to_string(std::clamp(gridSize, 16u, 256u)) +
            ",\"jacobi_iters\":" + std::to_string(std::min(jacobiIterations, 128u)) +
            ",\"cutoff_factor\":" + std::to_string(std::clamp(cutoffFactor, 0.0f, 8.0f)) +
            ",\"max_local_neighbors\":" + std::to_string(std::min(maxLocalNeighbors, 256u)) +
            ",\"particle_limit\":" + std::to_string(std::min(particleLimit, 100000000u)) +
            ",\"dense_cell_threshold\":" +
            std::to_string(std::clamp(denseCellThreshold, 1u, 4096u)) +
            ",\"gravity_only_buffers\":" + (gravityOnlyBuffers ? "true" : "false"));
}

void Bridge::setAdaptiveTimeSteps(bool enabled, std::uint32_t maxLevel, float eta)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    sendOrQueueRemote(std::string(bltzr_protocol::SetAdaptiveTimeSteps),
                      std::string("\"enabled\":") + (enabled ? "true" : "false") +
                          ",\"max_level\":" + std::to_string(std::min(12u, maxLevel)) +
                          ",\"eta\":" + std::to_string(std::clamp(eta, 0.01f, 1.0f)));
}

void Bridge::setAdaptiveTimeStepCostGuard(bool enabled)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    sendOrQueueRemote(std::string(bltzr_protocol::SetAdaptiveTimeStepCostGuard),
                      std::string("\"enabled\":") + (enabled ? "true" : "false"));
}

void Bridge::setOctreeParameters(float theta, float softening)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    sendOrQueueRemote(
        std::string(bltzr_protocol::SetOctree),
        "\"theta\":" + std::to_string(std::max(kPhysicsMinTheta, theta)) +
            ",\"softening\":" + std::to_string(std::max(kMinSimulationDt, softening)));
}

void Bridge::setSphEnabled(bool enabled)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    sendOrQueueRemote(std::string(bltzr_protocol::SetSph),
                      std::string("\"value\":") + (enabled ? "true" : "false"));
}

void Bridge::setSphParameters(float smoothingLength, float restDensity, float gasConstant,
                              float viscosity)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    sendOrQueueRemote(std::string(bltzr_protocol::SetSphParams),
                      "\"h\":" + std::to_string(std::max(0.000001f, smoothingLength)) +
                          ",\"rest_density\":" + std::to_string(std::max(0.000001f, restDensity)) +
                          ",\"gas_constant\":" + std::to_string(std::max(0.000001f, gasConstant)) +
                          ",\"viscosity\":" + std::to_string(std::max(0.0f, viscosity)));
}

void Bridge::setSubstepPolicy(float targetDt, std::uint32_t maxSubsteps)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    sendOrQueueRemote(
        std::string(bltzr_protocol::SetSubsteps),
        "\"target_dt\":" + std::to_string(std::max(0.0f, targetDt)) +
            ",\"max_substeps\":" + std::to_string(std::max<std::uint32_t>(1u, maxSubsteps)));
}

void Bridge::setSnapshotPublishPeriodMs(std::uint32_t periodMs)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    sendOrQueueRemote(std::string(bltzr_protocol::SetSnapshotPublishCadence),
                      "\"period_ms\":" + std::to_string(std::max<std::uint32_t>(1u, periodMs)));
}

void Bridge::setEnergyMeasurementConfig(std::uint32_t everySteps, std::uint32_t sampleLimit)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    sendOrQueueRemote(std::string(bltzr_protocol::SetEnergyMeasure),
                      "\"every_steps\":" + std::to_string(std::max(1u, everySteps)) +
                          ",\"sample_limit\":" + std::to_string(std::max(2u, sampleLimit)));
}

void Bridge::setGpuTelemetryEnabled(bool enabled)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    sendOrQueueRemote(std::string(bltzr_protocol::SetGpuTelemetry),
                      std::string("\"value\":") + (enabled ? "true" : "false"));
}

void Bridge::setExportDefaults(const std::string& directory, const std::string& format)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    static_cast<void>(directory);
    _defaultExportFormat = format;
}

void Bridge::setInitialStateFile(const std::string& path, const std::string& format)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (!path.empty()) {
        sendOrQueueRemote(std::string(bltzr_protocol::Load),
                          "\"path\":\"" + jsonEscape(path) + "\",\"format\":\"" +
                              jsonEscape(format.empty() ? "auto" : format) + "\"");
    }
}

void Bridge::requestExportSnapshot(const std::string& outputPath, const std::string& format)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    const std::string effectiveFormat = format.empty() ? _defaultExportFormat : format;
    std::string fields;
    if (!outputPath.empty()) {
        fields = "\"path\":\"" + jsonEscape(outputPath) + "\"";
    }
    if (!effectiveFormat.empty()) {
        if (!fields.empty()) {
            fields += ",";
        }
        fields += "\"format\":\"" + jsonEscape(effectiveFormat) + "\"";
    }
    sendOrQueueRemote(std::string(bltzr_protocol::Export), fields);
}

void Bridge::requestSaveCheckpoint(const std::string& outputPath)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    sendOrQueueRemote(std::string(bltzr_protocol::SaveCheckpoint),
                      "\"path\":\"" + jsonEscape(outputPath) + "\"");
}

void Bridge::requestLoadCheckpoint(const std::string& inputPath)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    sendOrQueueRemote(std::string(bltzr_protocol::LoadCheckpoint),
                      "\"path\":\"" + jsonEscape(inputPath) + "\"");
}

void Bridge::requestShutdown()
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    sendOrQueueRemote(std::string(bltzr_protocol::Shutdown));
}
} // namespace bltzr_client
