#ifndef GRAVITY_RUNTIME_INCLUDE_FRONTEND_IFRONTENDRUNTIME_HPP_
#define GRAVITY_RUNTIME_INCLUDE_FRONTEND_IFRONTENDRUNTIME_HPP_

#include "types/SimulationTypes.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace grav_frontend {

struct ConsumedSnapshot final {
    std::vector<RenderParticle> particles;
    std::size_t sourceSize = 0u;
};

class IFrontendRuntime {
    public:
        virtual ~IFrontendRuntime() = default;

        virtual bool start() = 0;
        virtual void stop() = 0;

        virtual void setPaused(bool paused) = 0;
        virtual void togglePaused() = 0;
        virtual void stepOnce() = 0;
        virtual void setParticleCount(std::uint32_t particleCount) = 0;
        virtual void setDt(float dt) = 0;
        virtual void scaleDt(float factor) = 0;
        virtual void requestReset() = 0;
        virtual void requestRecover() = 0;
        virtual void setSolverMode(const std::string &mode) = 0;
        virtual void setIntegratorMode(const std::string &mode) = 0;
        virtual void setOctreeParameters(float theta, float softening) = 0;
        virtual void setSphEnabled(bool enabled) = 0;
        virtual void setSphParameters(float smoothingLength, float restDensity, float gasConstant, float viscosity) = 0;
        virtual void setInitialStateConfig(const InitialStateConfig &config) = 0;
        virtual void setEnergyMeasurementConfig(std::uint32_t everySteps, std::uint32_t sampleLimit) = 0;
        virtual void setExportDefaults(const std::string &directory, const std::string &format) = 0;
        virtual void setInitialStateFile(const std::string &path, const std::string &format) = 0;
        virtual void requestExportSnapshot(const std::string &outputPath, const std::string &format) = 0;
        virtual void requestShutdown() = 0;
        virtual void setRemoteSnapshotCap(std::uint32_t maxPoints) = 0;
        virtual void requestReconnect() = 0;
        virtual void configureRemoteConnector(
            const std::string &host,
            std::uint16_t port,
            bool autoStart,
            const std::string &backendExecutable
        ) = 0;
        virtual bool isRemoteMode() const = 0;

        virtual SimulationStats getCachedStats() const = 0;
        virtual SimulationStats getStats() const = 0;
        virtual std::optional<ConsumedSnapshot> consumeLatestSnapshot() = 0;
        virtual bool tryConsumeSnapshot(std::vector<RenderParticle> &outSnapshot) = 0;
        virtual std::string linkStateLabel() const = 0;
        virtual std::string backendOwnerLabel() const = 0;
        virtual std::uint32_t statsAgeMs() const = 0;
        virtual std::uint32_t snapshotAgeMs() const = 0;
};

} // namespace grav_frontend



#endif // GRAVITY_RUNTIME_INCLUDE_FRONTEND_IFRONTENDRUNTIME_HPP_
