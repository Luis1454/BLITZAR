/*
 * @file engine/server/src/simulation/runtime/Export.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Simulation server runtime export operations.
 */

#include "simulation/Internal.hpp"

/*
 * @brief Documents the set export defaults operation contract.
 * @param directory Input value used by this contract.
 * @param format Input value used by this contract.
 * @return void SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void SimulationServer::setExportDefaults(const std::string& directory, const std::string& format)
{
    std::lock_guard<std::mutex> lock(_commandMutex);
    if (!directory.empty()) {
        _configState._exportDirectory = directory;
    }
    if (!format.empty()) {
        _configState._exportFormatDefault = format;
    }
}

/*
 * @brief Documents the request export snapshot operation contract.
 * @param outputPath Input value used by this contract.
 * @param format Input value used by this contract.
 * @return void SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void SimulationServer::requestExportSnapshot(const std::string& outputPath,
                                             const std::string& format)
{
    {
        std::lock_guard<std::mutex> lock(_commandMutex);
        PendingExportRequest request{};
        request.outputPath = outputPath;
        request.format = format.empty() ? _configState._exportFormatDefault : format;
        _pendingExportRequests.push_back(std::move(request));
    }
    _exportQueueDepth.fetch_add(1u, std::memory_order_relaxed);
    updateExportStatus("queued", outputPath, "queued for background write");
}

/*
 * @brief Documents the start export worker operation contract.
 * @param None This contract does not take explicit parameters.
 * @return void SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void SimulationServer::startExportWorker()
{
    if (_exportQueueState == nullptr || _exportQueueState->worker.joinable()) {
        return;
    }
    _exportQueueState->stopRequested = false;
    _exportQueueState->worker = std::thread([this]() {
        bool workerDone = false;
        while (!workerDone) {
            AsyncExportJob job{};
            {
                std::unique_lock<std::mutex> lock(_exportQueueState->mutex);
                _exportQueueState->condition.wait(lock, [this]() {
                    return _exportQueueState->stopRequested || !_exportQueueState->jobs.empty();
                });
                if (_exportQueueState->jobs.empty()) {
                    workerDone = _exportQueueState->stopRequested;
                    continue;
                }
                job = std::move(_exportQueueState->jobs.front());
                _exportQueueState->jobs.pop_front();
            }
            _exportActive.store(true, std::memory_order_relaxed);
            updateExportStatus("writing", job.outputPath, "writing snapshot on background worker");
            const bool ok = writeExportSnapshotFile(job);
            _exportActive.store(false, std::memory_order_relaxed);
            _exportQueueDepth.fetch_sub(1u, std::memory_order_relaxed);
            if (ok) {
                _exportCompletedCount.fetch_add(1u, std::memory_order_relaxed);
                updateExportStatus("completed", job.outputPath, "background export finished");
                std::cout << "[server] export ok: " << job.outputPath << "\n";
            }
            else {
                _exportFailedCount.fetch_add(1u, std::memory_order_relaxed);
                updateExportStatus("failed", job.outputPath, "background export failed");
                std::cerr << "[server] export failed: " << job.outputPath << "\n";
            }
        }
    });
}
