// File: engine/src/server/simulation_server/TelemetryAndPendingOps.cpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#include "Internal.hpp"
void SimulationServer::clearGpuTelemetry()
{
    _gpuTelemetryAvailable.store(false, std::memory_order_relaxed);
    _gpuKernelMs.store(0.0f, std::memory_order_relaxed);
    _gpuCopyMs.store(0.0f, std::memory_order_relaxed);
    _gpuVramUsedBytes.store(0u, std::memory_order_relaxed);
    _gpuVramTotalBytes.store(0u, std::memory_order_relaxed);
}
void SimulationServer::maybeSampleGpuTelemetry(std::string_view solverMode,
                                               std::uint64_t currentStep)
{
    if (!_gpuTelemetryEnabled.load(std::memory_order_relaxed)) {
        return;
    }
    if (solverMode != grav_modes::kSolverPairwiseCuda &&
        solverMode != grav_modes::kSolverOctreeGpu) {
        _gpuTelemetryAvailable.store(false, std::memory_order_relaxed);
        _gpuKernelMs.store(0.0f, std::memory_order_relaxed);
        _gpuCopyMs.store(0.0f, std::memory_order_relaxed);
        _gpuVramUsedBytes.store(0u, std::memory_order_relaxed);
        _gpuVramTotalBytes.store(0u, std::memory_order_relaxed);
        return;
    }
    if ((currentStep % kGpuTelemetrySampleStride) != 0u) {
        return;
    }
    std::size_t freeBytes = 0u;
    std::size_t totalBytes = 0u;
    const cudaError_t status = cudaMemGetInfo(&freeBytes, &totalBytes);
    if (status != cudaSuccess) {
        _gpuTelemetryAvailable.store(false, std::memory_order_relaxed);
        _gpuVramUsedBytes.store(0u, std::memory_order_relaxed);
        _gpuVramTotalBytes.store(0u, std::memory_order_relaxed);
        return;
    }
    _gpuTelemetryAvailable.store(true, std::memory_order_relaxed);
    _gpuVramUsedBytes.store(
        static_cast<std::uint64_t>(totalBytes >= freeBytes ? (totalBytes - freeBytes) : 0u),
        std::memory_order_relaxed);
    _gpuVramTotalBytes.store(static_cast<std::uint64_t>(totalBytes), std::memory_order_relaxed);
}
void SimulationServer::processPendingExport()
{
    PendingExportRequest request{};
    bool hasRequest = false;
    std::string outputPath;
    std::string format;
    std::string exportDirectory;
    {
        std::lock_guard<std::mutex> lock(_commandMutex);
        if (!_pendingExportRequests.empty()) {
            request = std::move(_pendingExportRequests.front());
            _pendingExportRequests.pop_front();
            hasRequest = true;
        }
        exportDirectory = _exportDirectory;
    }
    if (!hasRequest) {
        return;
    }
    outputPath = request.outputPath;
    format = request.format.empty() ? _exportFormatDefault : request.format;
    if (outputPath.empty()) {
        outputPath =
            defaultExportPath(exportDirectory, format, _steps.load(std::memory_order_relaxed));
    }
    if (exportCurrentState(outputPath, format)) {
        updateExportStatus("captured", outputPath, "snapshot captured and queued");
    }
    else {
        _exportQueueDepth.fetch_sub(1u, std::memory_order_relaxed);
        _exportFailedCount.fetch_add(1u, std::memory_order_relaxed);
        updateExportStatus("failed", outputPath, "could not capture snapshot for export");
        std::cerr << "[server] export capture failed: " << outputPath << "\n";
    }
}
void SimulationServer::processPendingCheckpointSave()
{
    if (_checkpointQueueState == nullptr) {
        return;
    }
    PendingCheckpointSaveRequest request{};
    bool hasRequest = false;
    {
        std::lock_guard<std::mutex> lock(_checkpointQueueState->mutex);
        if (!_checkpointQueueState->saveRequests.empty()) {
            request = std::move(_checkpointQueueState->saveRequests.front());
            _checkpointQueueState->saveRequests.pop_front();
            hasRequest = true;
        }
    }
    if (!hasRequest || request.result == nullptr) {
        return;
    }
    std::string error;
    const bool ok = captureCheckpointToFile(request.outputPath, &error);
    {
        std::lock_guard<std::mutex> lock(request.result->mutex);
        request.result->completed = true;
        request.result->ok = ok;
        request.result->error = error;
    }
    request.result->condition.notify_all();
}
bool SimulationServer::exportCurrentState(const std::string& outputPath, const std::string& format)
{
    if (!_system)
        return false;
    if (!_system->syncHostState()) {
        return false;
    }
    const std::vector<Particle>& particles = _system->getParticles();
    std::string solverModeLabel;
    std::string integratorModeLabel;
    {
        std::lock_guard<std::mutex> lock(_commandMutex);
        solverModeLabel = _solverMode;
        integratorModeLabel = _integratorMode;
    }
    enqueueExportWrite(outputPath, format, particles, solverModeLabel, integratorModeLabel,
                       _steps.load(std::memory_order_relaxed));
    return true;
}
