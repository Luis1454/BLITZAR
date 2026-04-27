// File: engine/src/config/SimulationProfile.cpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#include "config/SimulationProfile.hpp"
#include "config/SimulationConfig.hpp"
#include <algorithm>
#include <cctype>
namespace grav_config {
static std::string toLowerProfile(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}
bool normalizeSimulationProfile(std::string_view raw, std::string& outCanonical)
{
    const std::string lowered = toLowerProfile(std::string(raw));
    if (lowered == kSimulationProfileDiskOrbit) {
        outCanonical = std::string(kSimulationProfileDiskOrbit);
        return true;
    }
    if (lowered == kSimulationProfileGalaxyCollision) {
        outCanonical = std::string(kSimulationProfileGalaxyCollision);
        return true;
    }
    if (lowered == kSimulationProfilePlummerSphere) {
        outCanonical = std::string(kSimulationProfilePlummerSphere);
        return true;
    }
    if (lowered == kSimulationProfileBinaryStar) {
        outCanonical = std::string(kSimulationProfileBinaryStar);
        return true;
    }
    if (lowered == kSimulationProfileSolarSystem) {
        outCanonical = std::string(kSimulationProfileSolarSystem);
        return true;
    }
    if (lowered == kSimulationProfileSphCollapse) {
        outCanonical = std::string(kSimulationProfileSphCollapse);
        return true;
    }
    return false;
}
void applySimulationProfile(SimulationConfig& config)
{
    std::string canonical;
    if (!normalizeSimulationProfile(config.simulationProfile, canonical)) {
        return;
    }
    config.simulationProfile = canonical;
    if (canonical == kSimulationProfileDiskOrbit) {
        config.particleCount = 10000u;
        config.dt = 0.01f;
        config.solver = "pairwise_cuda";
        config.integrator = "euler";
        config.initMode = "disk_orbit";
        config.initConfigStyle = "preset";
        config.presetStructure = "disk_orbit";
    }
    else if (canonical == kSimulationProfileGalaxyCollision) {
        config.particleCount = 40000u;
        config.dt = 0.02f;
        config.solver = "octree_gpu";
        config.integrator = "euler";
        config.initMode = "galaxy_collision";
        config.initConfigStyle = "preset";
        config.presetStructure = "galaxy_collision";
        config.octreeTheta = 0.9f;
        config.octreeSoftening = 1.0f;
    }
    else if (canonical == kSimulationProfilePlummerSphere) {
        config.particleCount = 16384u;
        config.dt = 0.005f;
        config.solver = "octree_gpu";
        config.integrator = "euler";
        config.initMode = "plummer_sphere";
        config.initConfigStyle = "preset";
        config.presetStructure = "plummer_sphere";
    }
    else if (canonical == kSimulationProfileBinaryStar) {
        config.particleCount = 2u;
        config.dt = 0.001f;
        config.solver = "pairwise_cuda";
        config.integrator = "rk4";
        config.initMode = "two_body";
        config.initConfigStyle = "preset";
        config.presetStructure = "two_body";
    }
    else if (canonical == kSimulationProfileSolarSystem) {
        config.particleCount = 10u;
        config.dt = 0.0001f;
        config.solver = "pairwise_cuda";
        config.integrator = "rk4";
        config.initMode = "disk_orbit"; // Placeholder until solar system generator
        config.initConfigStyle = "preset";
    }
    else if (canonical == kSimulationProfileSphCollapse) {
        config.particleCount = 20000u;
        config.dt = 0.005f;
        config.solver = "octree_gpu";
        config.integrator = "euler";
        config.initMode = "random_cloud";
        config.initConfigStyle = "preset";
        config.sphEnabled = true;
    }
}
} // namespace grav_config
