#include "config/SimulationArgsInitStateOptions.hpp"
#include "config/SimulationArgsParse.hpp"

bool SimulationArgsInitStateOptions::apply(
    const std::string &key,
    const std::string &value,
    SimulationConfig &config,
    RuntimeArgs &runtime,
    std::ostream &warnings
)
{
    if (key == "--init-mode") {
        config.initMode = value;
        return true;
    }
    if (key == "--init-seed") {
        std::uint32_t parsedValue = config.initSeed;
        if (SimulationArgsParse::parseUint(value, parsedValue)) {
            config.initSeed = parsedValue;
        } else {
            warnings << "[args] invalid --init-seed: " << value << "\n";
        }
        return true;
    }
    if (key == "--init-include-central-body") {
        bool parsedValue = config.initIncludeCentralBody;
        if (SimulationArgsParse::parseBool(value, parsedValue)) {
            config.initIncludeCentralBody = parsedValue;
        } else {
            warnings << "[args] invalid --init-include-central-body: " << value << "\n";
        }
        return true;
    }
    if (key == "--init-central-mass") {
        float parsedValue = config.initCentralMass;
        if (SimulationArgsParse::parseFloat(value, parsedValue) && parsedValue > 0.0f) {
            config.initCentralMass = parsedValue;
        } else {
            warnings << "[args] invalid --init-central-mass: " << value << "\n";
        }
        return true;
    }
    if (key == "--init-central-x") {
        float parsedValue = config.initCentralX;
        if (SimulationArgsParse::parseFloat(value, parsedValue)) {
            config.initCentralX = parsedValue;
        } else {
            warnings << "[args] invalid --init-central-x: " << value << "\n";
        }
        return true;
    }
    if (key == "--init-central-y") {
        float parsedValue = config.initCentralY;
        if (SimulationArgsParse::parseFloat(value, parsedValue)) {
            config.initCentralY = parsedValue;
        } else {
            warnings << "[args] invalid --init-central-y: " << value << "\n";
        }
        return true;
    }
    if (key == "--init-central-z") {
        float parsedValue = config.initCentralZ;
        if (SimulationArgsParse::parseFloat(value, parsedValue)) {
            config.initCentralZ = parsedValue;
        } else {
            warnings << "[args] invalid --init-central-z: " << value << "\n";
        }
        return true;
    }
    if (key == "--init-central-vx") {
        float parsedValue = config.initCentralVx;
        if (SimulationArgsParse::parseFloat(value, parsedValue)) {
            config.initCentralVx = parsedValue;
        } else {
            warnings << "[args] invalid --init-central-vx: " << value << "\n";
        }
        return true;
    }
    if (key == "--init-central-vy") {
        float parsedValue = config.initCentralVy;
        if (SimulationArgsParse::parseFloat(value, parsedValue)) {
            config.initCentralVy = parsedValue;
        } else {
            warnings << "[args] invalid --init-central-vy: " << value << "\n";
        }
        return true;
    }
    if (key == "--init-central-vz") {
        float parsedValue = config.initCentralVz;
        if (SimulationArgsParse::parseFloat(value, parsedValue)) {
            config.initCentralVz = parsedValue;
        } else {
            warnings << "[args] invalid --init-central-vz: " << value << "\n";
        }
        return true;
    }
    if (key == "--init-disk-mass") {
        float parsedValue = config.initDiskMass;
        if (SimulationArgsParse::parseFloat(value, parsedValue) && parsedValue > 0.0f) {
            config.initDiskMass = parsedValue;
        } else {
            warnings << "[args] invalid --init-disk-mass: " << value << "\n";
        }
        return true;
    }
    if (key == "--init-disk-radius-min") {
        float parsedValue = config.initDiskRadiusMin;
        if (SimulationArgsParse::parseFloat(value, parsedValue) && parsedValue > 0.0f) {
            config.initDiskRadiusMin = parsedValue;
        } else {
            warnings << "[args] invalid --init-disk-radius-min: " << value << "\n";
        }
        return true;
    }
    if (key == "--init-disk-radius-max") {
        float parsedValue = config.initDiskRadiusMax;
        if (SimulationArgsParse::parseFloat(value, parsedValue) && parsedValue > 0.0f) {
            config.initDiskRadiusMax = parsedValue;
        } else {
            warnings << "[args] invalid --init-disk-radius-max: " << value << "\n";
        }
        return true;
    }
    if (key == "--init-disk-thickness") {
        float parsedValue = config.initDiskThickness;
        if (SimulationArgsParse::parseFloat(value, parsedValue) && parsedValue >= 0.0f) {
            config.initDiskThickness = parsedValue;
        } else {
            warnings << "[args] invalid --init-disk-thickness: " << value << "\n";
        }
        return true;
    }
    if (key == "--init-velocity-scale") {
        float parsedValue = config.initVelocityScale;
        if (SimulationArgsParse::parseFloat(value, parsedValue) && parsedValue >= 0.0f) {
            config.initVelocityScale = parsedValue;
        } else {
            warnings << "[args] invalid --init-velocity-scale: " << value << "\n";
        }
        return true;
    }
    if (key == "--init-cloud-half-extent") {
        float parsedValue = config.initCloudHalfExtent;
        if (SimulationArgsParse::parseFloat(value, parsedValue) && parsedValue > 0.0f) {
            config.initCloudHalfExtent = parsedValue;
        } else {
            warnings << "[args] invalid --init-cloud-half-extent: " << value << "\n";
        }
        return true;
    }
    if (key == "--init-cloud-speed") {
        float parsedValue = config.initCloudSpeed;
        if (SimulationArgsParse::parseFloat(value, parsedValue) && parsedValue >= 0.0f) {
            config.initCloudSpeed = parsedValue;
        } else {
            warnings << "[args] invalid --init-cloud-speed: " << value << "\n";
        }
        return true;
    }
    if (key == "--init-particle-mass") {
        float parsedValue = config.initParticleMass;
        if (SimulationArgsParse::parseFloat(value, parsedValue) && parsedValue > 0.0f) {
            config.initParticleMass = parsedValue;
        } else {
            warnings << "[args] invalid --init-particle-mass: " << value << "\n";
        }
        return true;
    }
    if (key == "--target-steps") {
        int parsedValue = runtime.targetSteps;
        if (SimulationArgsParse::parseInt(value, parsedValue) && parsedValue > 0) {
            runtime.targetSteps = parsedValue;
        } else {
            warnings << "[args] invalid --target-steps: " << value << "\n";
        }
        return true;
    }
    return false;
}
