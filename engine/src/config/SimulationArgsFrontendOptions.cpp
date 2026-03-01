#include "config/SimulationArgsFrontendOptions.hpp"
#include "config/SimulationArgsParse.hpp"

bool SimulationArgsFrontendOptions::apply(
    const std::string &key,
    const std::string &value,
    SimulationConfig &config,
    std::ostream &warnings
)
{
    if (key == "--zoom") {
        float parsedValue = config.defaultZoom;
        if (SimulationArgsParse::parseFloat(value, parsedValue) && parsedValue > 0.01f) {
            config.defaultZoom = parsedValue;
        } else {
            warnings << "[args] invalid --zoom: " << value << "\n";
        }
        return true;
    }
    if (key == "--luminosity") {
        int parsedValue = config.defaultLuminosity;
        if (SimulationArgsParse::parseInt(value, parsedValue) && parsedValue >= 0 && parsedValue <= 255) {
            config.defaultLuminosity = parsedValue;
        } else {
            warnings << "[args] invalid --luminosity: " << value << "\n";
        }
        return true;
    }
    if (key == "--ui-fps") {
        std::uint32_t parsedValue = config.uiFpsLimit;
        if (SimulationArgsParse::parseUint(value, parsedValue) && parsedValue >= 1u) {
            config.uiFpsLimit = parsedValue;
        } else {
            warnings << "[args] invalid --ui-fps: " << value << "\n";
        }
        return true;
    }
    if (key == "--backend-timeout-ms") {
        std::uint32_t parsedValue = config.frontendRemoteCommandTimeoutMs;
        if (SimulationArgsParse::parseUint(value, parsedValue) && parsedValue >= 10u && parsedValue <= 60000u) {
            config.frontendRemoteCommandTimeoutMs = parsedValue;
            config.frontendRemoteStatusTimeoutMs = parsedValue;
            config.frontendRemoteSnapshotTimeoutMs = parsedValue;
        } else {
            warnings << "[args] invalid --backend-timeout-ms: " << value << "\n";
        }
        return true;
    }
    if (key == "--backend-command-timeout-ms") {
        std::uint32_t parsedValue = config.frontendRemoteCommandTimeoutMs;
        if (SimulationArgsParse::parseUint(value, parsedValue) && parsedValue >= 10u && parsedValue <= 60000u) {
            config.frontendRemoteCommandTimeoutMs = parsedValue;
        } else {
            warnings << "[args] invalid --backend-command-timeout-ms: " << value << "\n";
        }
        return true;
    }
    if (key == "--backend-status-timeout-ms") {
        std::uint32_t parsedValue = config.frontendRemoteStatusTimeoutMs;
        if (SimulationArgsParse::parseUint(value, parsedValue) && parsedValue >= 10u && parsedValue <= 60000u) {
            config.frontendRemoteStatusTimeoutMs = parsedValue;
        } else {
            warnings << "[args] invalid --backend-status-timeout-ms: " << value << "\n";
        }
        return true;
    }
    if (key == "--backend-snapshot-timeout-ms") {
        std::uint32_t parsedValue = config.frontendRemoteSnapshotTimeoutMs;
        if (SimulationArgsParse::parseUint(value, parsedValue) && parsedValue >= 10u && parsedValue <= 60000u) {
            config.frontendRemoteSnapshotTimeoutMs = parsedValue;
        } else {
            warnings << "[args] invalid --backend-snapshot-timeout-ms: " << value << "\n";
        }
        return true;
    }
    if (key == "--export-directory") {
        config.exportDirectory = value;
        return true;
    }
    if (key == "--export-format") {
        config.exportFormat = value;
        return true;
    }
    if (key == "--input-file") {
        config.inputFile = value;
        return true;
    }
    if (key == "--input-format") {
        config.inputFormat = value;
        return true;
    }
    if (key == "--init-config-style") {
        config.initConfigStyle = value;
        return true;
    }
    if (key == "--preset-structure" || key == "--structure") {
        config.presetStructure = value;
        return true;
    }
    if (key == "--preset-size" || key == "--size") {
        float parsedValue = config.presetSize;
        if (SimulationArgsParse::parseFloat(value, parsedValue) && parsedValue > 0.01f) {
            config.presetSize = parsedValue;
        } else {
            warnings << "[args] invalid " << key << ": " << value << "\n";
        }
        return true;
    }
    if (key == "--velocity-temperature") {
        float parsedValue = config.velocityTemperature;
        if (SimulationArgsParse::parseFloat(value, parsedValue) && parsedValue >= 0.0f) {
            config.velocityTemperature = parsedValue;
        } else {
            warnings << "[args] invalid --velocity-temperature: " << value << "\n";
        }
        return true;
    }
    if (key == "--particle-temperature") {
        float parsedValue = config.particleTemperature;
        if (SimulationArgsParse::parseFloat(value, parsedValue) && parsedValue >= 0.0f) {
            config.particleTemperature = parsedValue;
        } else {
            warnings << "[args] invalid --particle-temperature: " << value << "\n";
        }
        return true;
    }
    if (key == "--thermal-ambient") {
        float parsedValue = config.thermalAmbientTemperature;
        if (SimulationArgsParse::parseFloat(value, parsedValue) && parsedValue >= 0.0f) {
            config.thermalAmbientTemperature = parsedValue;
        } else {
            warnings << "[args] invalid --thermal-ambient: " << value << "\n";
        }
        return true;
    }
    if (key == "--thermal-specific-heat") {
        float parsedValue = config.thermalSpecificHeat;
        if (SimulationArgsParse::parseFloat(value, parsedValue) && parsedValue > 0.0f) {
            config.thermalSpecificHeat = parsedValue;
        } else {
            warnings << "[args] invalid --thermal-specific-heat: " << value << "\n";
        }
        return true;
    }
    if (key == "--thermal-heating") {
        float parsedValue = config.thermalHeatingCoeff;
        if (SimulationArgsParse::parseFloat(value, parsedValue) && parsedValue >= 0.0f) {
            config.thermalHeatingCoeff = parsedValue;
        } else {
            warnings << "[args] invalid --thermal-heating: " << value << "\n";
        }
        return true;
    }
    if (key == "--thermal-radiation") {
        float parsedValue = config.thermalRadiationCoeff;
        if (SimulationArgsParse::parseFloat(value, parsedValue) && parsedValue >= 0.0f) {
            config.thermalRadiationCoeff = parsedValue;
        } else {
            warnings << "[args] invalid --thermal-radiation: " << value << "\n";
        }
        return true;
    }
    return false;
}
