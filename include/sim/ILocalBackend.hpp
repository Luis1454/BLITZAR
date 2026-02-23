#ifndef GRAVITY_SIM_ILOCALBACKEND_HPP
#define GRAVITY_SIM_ILOCALBACKEND_HPP

#include "sim/SimulationTypes.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace sim {

class ILocalBackend {
    public:
        virtual ~ILocalBackend() = default;

        virtual void start() = 0;
        virtual void stop() = 0;
        virtual void setPaused(bool paused) = 0;
        virtual void togglePaused() = 0;
        virtual void stepOnce() = 0;
        virtual void setParticleCount(std::uint32_t particleCount) = 0;
        virtual void setDt(float dt) = 0;
        virtual void scaleDt(float factor) = 0;
        virtual void requestReset() = 0;
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
        virtual bool tryConsumeSnapshot(std::vector<RenderParticle> &outSnapshot) = 0;
        virtual SimulationStats getStats() const = 0;
};

} // namespace sim

#endif // GRAVITY_SIM_ILOCALBACKEND_HPP
