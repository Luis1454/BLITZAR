/*
 * @file runtime/src/client/runtime/InitialState.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Initial-state command serialization for the client facade.
 */

#include "client/runtime/Bridge.hpp"
#include "core/Config.hpp"
#include "directive/Config.hpp"
#include "protocol/Protocol.hpp"
#include <sstream>

namespace bltzr_client {
static SimulationConfig makeInitialStateEnvelope(const InitialStateConfig& state)
{
    SimulationConfig config = SimulationConfig::defaults();
    config.initConfigStyle = "detailed";
    config.initMode = state.mode;
    config.scene = state.scene;
    config.initSeed = state.seed;
    config.deterministicMode = state.deterministicMode;
    config.velocityTemperature = state.velocityTemperature;
    config.particleTemperature = state.particleTemperature;
    config.thermalAmbientTemperature = state.thermalAmbientTemperature;
    config.thermalSpecificHeat = state.thermalSpecificHeat;
    config.thermalHeatingCoeff = state.thermalHeatingCoeff;
    config.thermalRadiationCoeff = state.thermalRadiationCoeff;
    config.initIncludeCentralBody = state.includeCentralBody;
    config.initCentralMass = state.centralMass;
    config.initCentralX = state.centralX;
    config.initCentralY = state.centralY;
    config.initCentralZ = state.centralZ;
    config.initCentralVx = state.centralVx;
    config.initCentralVy = state.centralVy;
    config.initCentralVz = state.centralVz;
    config.initDiskMass = state.diskMass;
    config.initDiskRadiusMin = state.diskRadiusMin;
    config.initDiskRadiusMax = state.diskRadiusMax;
    config.initDiskThickness = state.diskThickness;
    config.initVelocityScale = state.velocityScale;
    config.initCloudHalfExtent = state.cloudHalfExtent;
    config.initCubeHalfExtent = state.cubeHalfExtent;
    config.initSphereRadius = state.sphereRadius;
    config.initCloudSpeed = state.cloudSpeed;
    config.initParticleMass = state.particleMass;
    config.sceneOffsetX = state.sceneOffsetX;
    config.sceneOffsetY = state.sceneOffsetY;
    config.sceneOffsetZ = state.sceneOffsetZ;
    config.sceneRotationX = state.sceneRotationX;
    config.sceneRotationY = state.sceneRotationY;
    config.sceneRotationZ = state.sceneRotationZ;
    config.sceneCopyAxis = state.sceneCopyAxis;
    config.sceneRotationCopies = state.sceneRotationCopies;
    config.sceneMirrorX = state.sceneMirrorX;
    config.sceneMirrorY = state.sceneMirrorY;
    config.sceneMirrorZ = state.sceneMirrorZ;
    return config;
}

static std::string serializeInitialStateConfig(const InitialStateConfig& state)
{
    std::ostringstream output;
    bltzr_config::SimulationConfigDirective::write(output, makeInitialStateEnvelope(state));
    return output.str();
}

void Bridge::setInitialStateConfig(const InitialStateConfig& config)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (config.mode == "file") {
        return;
    }
    sendOrQueueRemote(std::string(bltzr_protocol::SetInitialStateConfig),
                      "\"config\":\"" + jsonEscape(serializeInitialStateConfig(config)) + "\"");
}
} // namespace bltzr_client
