#include "config/SimulationModes.hpp"
#include "config/SimulationArgsCoreOptions.hpp"
#include "config/SimulationArgsParse.hpp"

bool SimulationArgsCoreOptions::apply(
    const std::string &key,
    const std::string &value,
    SimulationConfig &config,
    RuntimeArgs &runtime,
    std::ostream &warnings
)
{
    if (key == "--config") {
        runtime.configPath = value;
        return true;
    }
    if (key == "--particle-count") {
        std::uint32_t parsedValue = config.particleCount;
        if (SimulationArgsParse::parseUint(value, parsedValue) && parsedValue >= 2u) {
            config.particleCount = parsedValue;
        } else {
            warnings << "[args] invalid --particle-count: " << value << "\n";
        }
        return true;
    }
    if (key == "--dt") {
        float parsedValue = config.dt;
        if (SimulationArgsParse::parseFloat(value, parsedValue) && parsedValue > 0.0f) {
            config.dt = parsedValue;
        } else {
            warnings << "[args] invalid --dt: " << value << "\n";
        }
        return true;
    }
    if (key == "--solver") {
        std::string canonical;
        if (grav_modes::normalizeSolver(value, canonical)) {
            config.solver = canonical;
        } else {
            runtime.hasArgumentError = true;
            warnings << "[args] invalid --solver: " << value
                     << " (allowed: pairwise_cuda|octree_gpu|octree_cpu)\n";
        }
        return true;
    }
    if (key == "--integrator") {
        std::string canonical;
        if (grav_modes::normalizeIntegrator(value, canonical)) {
            config.integrator = canonical;
        } else {
            runtime.hasArgumentError = true;
            warnings << "[args] invalid --integrator: " << value
                     << " (allowed: euler|rk4)\n";
        }
        return true;
    }
    if (key == "--octree-theta") {
        float parsedValue = config.octreeTheta;
        if (SimulationArgsParse::parseFloat(value, parsedValue) && parsedValue > 0.01f) {
            config.octreeTheta = parsedValue;
        } else {
            warnings << "[args] invalid --octree-theta: " << value << "\n";
        }
        return true;
    }
    if (key == "--octree-softening") {
        float parsedValue = config.octreeSoftening;
        if (SimulationArgsParse::parseFloat(value, parsedValue) && parsedValue > 0.000001f) {
            config.octreeSoftening = parsedValue;
        } else {
            warnings << "[args] invalid --octree-softening: " << value << "\n";
        }
        return true;
    }
    if (key == "--frontend-particle-cap") {
        std::uint32_t parsedValue = config.frontendParticleCap;
        if (SimulationArgsParse::parseUint(value, parsedValue) && parsedValue >= 2u) {
            config.frontendParticleCap = parsedValue;
        } else {
            warnings << "[args] invalid --frontend-particle-cap: " << value << "\n";
        }
        return true;
    }
    return false;
}
