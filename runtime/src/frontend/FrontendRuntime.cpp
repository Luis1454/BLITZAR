#include "frontend/FrontendRuntime.hpp"
#include "frontend/LocalBackendFactory.hpp"

#include <utility>

namespace grav_frontend {
FrontendRuntime::FrontendRuntime(const std::string &configPath, const FrontendTransportArgs &transport)
    : _bridge(
          configPath,
          transport.remoteMode,
          transport.remoteHost,
          transport.remotePort,
          transport.remoteAutoStart,
          transport.backendExecutable,
          transport.remoteAuthToken,
          transport.remoteCommandTimeoutMs,
          transport.remoteStatusTimeoutMs,
          transport.remoteSnapshotTimeoutMs,
          createLocalBackend),
      _pollThread(),
      _pollRunning(false),
      _dataMutex(),
      _latestStats(),
      _latestSnapshot(),
      _hasNewSnapshot(false),
      _latestSnapshotSize(0u),
      _latestLinkLabel(transport.remoteMode ? "reconnecting" : "local"),
      _latestOwnerLabel(transport.remoteMode ? "external" : "embedded"),
      _lastStatsAt(),
      _lastSnapshotAt(),
      _hasStats(false),
      _hasSnapshotEver(false)
{
}

FrontendRuntime::~FrontendRuntime()
{
    stop();
}

bool FrontendRuntime::start()
{
    stop();
    if (!_bridge.start()) {
        return false;
    }
    pollOnce(true, true);
    _pollRunning.store(true);
    _pollThread = std::thread([this]() { pollLoop(); });
    return true;
}

void FrontendRuntime::stop()
{
    _pollRunning.store(false);
    if (_pollThread.joinable()) {
        _pollThread.join();
    }
    _bridge.stop();
}

void FrontendRuntime::setPaused(bool paused)
{
    _bridge.setPaused(paused);
}

void FrontendRuntime::togglePaused()
{
    _bridge.togglePaused();
}

void FrontendRuntime::stepOnce()
{
    _bridge.stepOnce();
}

void FrontendRuntime::setParticleCount(std::uint32_t particleCount)
{
    _bridge.setParticleCount(particleCount);
}

void FrontendRuntime::setDt(float dt)
{
    _bridge.setDt(dt);
}

void FrontendRuntime::scaleDt(float factor)
{
    _bridge.scaleDt(factor);
}

void FrontendRuntime::requestReset()
{
    _bridge.requestReset();
}

void FrontendRuntime::requestRecover()
{
    _bridge.requestRecover();
}

void FrontendRuntime::setSolverMode(const std::string &mode)
{
    _bridge.setSolverMode(mode);
}

void FrontendRuntime::setIntegratorMode(const std::string &mode)
{
    _bridge.setIntegratorMode(mode);
}

void FrontendRuntime::setOctreeParameters(float theta, float softening)
{
    _bridge.setOctreeParameters(theta, softening);
}

void FrontendRuntime::setSphEnabled(bool enabled)
{
    _bridge.setSphEnabled(enabled);
}

void FrontendRuntime::setSphParameters(float smoothingLength, float restDensity, float gasConstant, float viscosity)
{
    _bridge.setSphParameters(smoothingLength, restDensity, gasConstant, viscosity);
}

void FrontendRuntime::setInitialStateConfig(const InitialStateConfig &config)
{
    _bridge.setInitialStateConfig(config);
}

void FrontendRuntime::setEnergyMeasurementConfig(std::uint32_t everySteps, std::uint32_t sampleLimit)
{
    _bridge.setEnergyMeasurementConfig(everySteps, sampleLimit);
}

void FrontendRuntime::setExportDefaults(const std::string &directory, const std::string &format)
{
    _bridge.setExportDefaults(directory, format);
}

void FrontendRuntime::setInitialStateFile(const std::string &path, const std::string &format)
{
    _bridge.setInitialStateFile(path, format);
}

void FrontendRuntime::requestExportSnapshot(const std::string &outputPath, const std::string &format)
{
    _bridge.requestExportSnapshot(outputPath, format);
}

void FrontendRuntime::requestShutdown()
{
    _bridge.requestShutdown();
}

void FrontendRuntime::setRemoteSnapshotCap(std::uint32_t maxPoints)
{
    _bridge.setRemoteSnapshotCap(maxPoints);
}

void FrontendRuntime::requestReconnect()
{
    _bridge.requestReconnect();
}

void FrontendRuntime::configureRemoteConnector(
    const std::string &host,
    std::uint16_t port,
    bool autoStart,
    const std::string &backendExecutable)
{
    _bridge.configureRemoteConnector(host, port, autoStart, backendExecutable);
}

bool FrontendRuntime::isRemoteMode() const
{
    return _bridge.isRemoteMode();
}

} // namespace grav_frontend
