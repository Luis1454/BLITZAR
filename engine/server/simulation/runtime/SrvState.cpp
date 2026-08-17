/*
 * @file engine/server/simulation/runtime/SrvState.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Simulation server runtime state operations.
 */

#include "server/simulation/runtime/SrvInternal.hpp"

/*
 * @brief Documents the set initial state config operation contract.
 * @param config Input value used by this contract.
 * @return void SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void SimulationServer::setInitialStateConfig(const InitialStateConfig& config)
{
    {
        std::lock_guard<std::mutex> lock(_commandMutex);
        _configState._initialStateConfig = config;
        if (config.mode != "file") {
            _configState._initialStatePath.clear();
            _configState._initialStateFormat = "auto";
            _configState._runtimeConfigMirror.inputFile.clear();
            _configState._runtimeConfigMirror.inputFormat = "auto";
        }
        _configState._runtimeConfigMirror.initMode = config.mode;
        _configState._runtimeConfigMirror.presetStructure = config.mode;
        _activeCheckpointState.reset();
    }
    if (_running.load(std::memory_order_relaxed)) {
        requestReset();
    }
}
/*
 * @brief Documents the set initial state file operation contract.
 * @param path Input value used by this contract.
 * @param format Input value used by this contract.
 * @return void SimulationServer:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void SimulationServer::setInitialStateFile(const std::string& path, const std::string& format)
{
    {
        std::lock_guard<std::mutex> lock(_commandMutex);
        _configState._initialStatePath = path;
        _configState._initialStateFormat = format.empty() ? "auto" : format;
        _activeCheckpointState.reset();
        _configState._runtimeConfigMirror.inputFile = _configState._initialStatePath;
        _configState._runtimeConfigMirror.inputFormat = _configState._initialStateFormat;
        if (!_configState._initialStatePath.empty()) {
            _configState._initialStateConfig.mode = "file";
            _configState._runtimeConfigMirror.initMode = "file";
            _configState._runtimeConfigMirror.presetStructure = "file";
        }
    }
    if (_running.load(std::memory_order_relaxed)) {
        requestReset();
    }
}
