#include "config/SimulationArgsFluidOptions.hpp"
#include "config/SimulationArgsParse.hpp"

bool SimulationArgsFluidOptions::apply(
    const std::string &key,
    const std::string &value,
    SimulationConfig &config,
    RuntimeArgs &runtime,
    std::ostream &warnings
)
{
    static_cast<void>(runtime);

    if (key == "--sph") {
        bool parsedValue = config.sphEnabled;
        if (SimulationArgsParse::parseBool(value, parsedValue)) {
            config.sphEnabled = parsedValue;
        } else {
            warnings << "[args] invalid --sph: " << value << "\n";
        }
        return true;
    }
    if (key == "--sph-h") {
        float parsedValue = config.sphSmoothingLength;
        if (SimulationArgsParse::parseFloat(value, parsedValue) && parsedValue > 0.05f) {
            config.sphSmoothingLength = parsedValue;
        } else {
            warnings << "[args] invalid --sph-h: " << value << "\n";
        }
        return true;
    }
    if (key == "--sph-rest-density") {
        float parsedValue = config.sphRestDensity;
        if (SimulationArgsParse::parseFloat(value, parsedValue) && parsedValue > 0.01f) {
            config.sphRestDensity = parsedValue;
        } else {
            warnings << "[args] invalid --sph-rest-density: " << value << "\n";
        }
        return true;
    }
    if (key == "--sph-gas-constant") {
        float parsedValue = config.sphGasConstant;
        if (SimulationArgsParse::parseFloat(value, parsedValue) && parsedValue > 0.01f) {
            config.sphGasConstant = parsedValue;
        } else {
            warnings << "[args] invalid --sph-gas-constant: " << value << "\n";
        }
        return true;
    }
    if (key == "--sph-viscosity") {
        float parsedValue = config.sphViscosity;
        if (SimulationArgsParse::parseFloat(value, parsedValue) && parsedValue >= 0.0f) {
            config.sphViscosity = parsedValue;
        } else {
            warnings << "[args] invalid --sph-viscosity: " << value << "\n";
        }
        return true;
    }
    if (key == "--energy-every") {
        std::uint32_t parsedValue = config.energyMeasureEverySteps;
        if (SimulationArgsParse::parseUint(value, parsedValue) && parsedValue >= 1u) {
            config.energyMeasureEverySteps = parsedValue;
        } else {
            warnings << "[args] invalid --energy-every: " << value << "\n";
        }
        return true;
    }
    if (key == "--energy-sample-limit") {
        std::uint32_t parsedValue = config.energySampleLimit;
        if (SimulationArgsParse::parseUint(value, parsedValue) && parsedValue >= 64u) {
            config.energySampleLimit = parsedValue;
        } else {
            warnings << "[args] invalid --energy-sample-limit: " << value << "\n";
        }
        return true;
    }
    return false;
}
