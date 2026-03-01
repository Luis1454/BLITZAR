#include "config/SimulationArgs.hpp"
#include "config/SimulationModes.hpp"
#include "config/TextParse.hpp"

#include <algorithm>
#include <cctype>
#include <limits>
#include <sstream>
static std::string toLower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

static bool parseBool(const std::string &value, bool &out)
{
    const std::string normalized = toLower(value);
    if (normalized == "1" || normalized == "true" || normalized == "on" || normalized == "yes") {
        out = true;
        return true;
    }
    if (normalized == "0" || normalized == "false" || normalized == "off" || normalized == "no") {
        out = false;
        return true;
    }
    return false;
}

static bool parseUint(const std::string &value, std::uint32_t &out)
{
    std::uint64_t parsed = 0;
    if (!grav_text::parseNumber(value, parsed)) {
        return false;
    }
    if (parsed > static_cast<std::uint64_t>(std::numeric_limits<std::uint32_t>::max())) {
        return false;
    }
    out = static_cast<std::uint32_t>(parsed);
    return true;
}

static bool parseInt(const std::string &value, int &out)
{
    long long parsed = 0;
    if (!grav_text::parseNumber(value, parsed)) {
        return false;
    }
    if (parsed < static_cast<long long>(std::numeric_limits<int>::min())
        || parsed > static_cast<long long>(std::numeric_limits<int>::max())) {
        return false;
    }
    out = static_cast<int>(parsed);
    return true;
}

static bool parseFloat(const std::string &value, float &out)
{
    return grav_text::parseNumber(value, out);
}

static bool splitOption(const std::string &raw, std::string &key, std::string &value)
{
    if (raw.rfind("--", 0) != 0) {
        return false;
    }
    const std::size_t eq = raw.find('=');
    if (eq == std::string::npos) {
        key = raw;
        value.clear();
        return true;
    }
    key = raw.substr(0, eq);
    value = raw.substr(eq + 1);
    return true;
}

static bool readValue(
    const std::vector<std::string_view> &args,
    std::size_t &index,
    const std::string &inlined,
    std::string &outValue
)
{
    if (!inlined.empty()) {
        outValue = inlined;
        return true;
    }
    if (index + 1 >= args.size() || args[index + 1].empty()) {
        return false;
    }
    outValue = std::string(args[++index]);
    return true;
}
std::string findConfigPathArg(const std::vector<std::string_view> &args, const std::string &fallback)
{
    for (std::size_t i = 1; i < args.size(); ++i) {
        if (args[i].empty()) {
            continue;
        }
        std::string key;
        std::string value;
        if (!splitOption(std::string(args[i]), key, value)) {
            continue;
        }
        if (key != "--config") {
            continue;
        }
        if (!value.empty()) {
            return value;
        }
        if (i + 1 < args.size() && !args[i + 1].empty()) {
            return std::string(args[i + 1]);
        }
        return fallback;
    }
    return fallback;
}

void applyArgsToConfig(
    const std::vector<std::string_view> &args,
    SimulationConfig &config,
    RuntimeArgs &runtime,
    std::ostream &warnings
)
{
    runtime.configPath = findConfigPathArg(args, runtime.configPath);

    for (std::size_t i = 1; i < args.size(); ++i) {
        if (args[i].empty()) {
            continue;
        }
        const std::string raw(args[i]);
        if (raw == "-h") {
            runtime.showHelp = true;
            continue;
        }
        std::string key;
        std::string inlineValue;
        if (!splitOption(raw, key, inlineValue)) {
            runtime.hasArgumentError = true;
            warnings << "[args] unexpected positional argument: " << raw << "\n";
            continue;
        }

        if (key == "--help" || key == "-h") {
            runtime.showHelp = true;
            continue;
        }
        if (key == "--save-config") {
            runtime.saveConfig = true;
            continue;
        }
        if (key == "--no-export-on-exit") {
            runtime.exportOnExit = false;
            continue;
        }
        if (key == "--export-on-exit") {
            bool parsed = true;
            bool value = true;
            if (!inlineValue.empty()) {
                parsed = parseBool(inlineValue, value);
            } else if (i + 1 < args.size() && !args[i + 1].empty() && std::string(args[i + 1]).rfind("--", 0) != 0) {
                std::string explicitValue(args[++i]);
                parsed = parseBool(explicitValue, value);
            }
            if (!parsed) {
                warnings << "[args] invalid bool for --export-on-exit\n";
            } else {
                runtime.exportOnExit = value;
            }
            continue;
        }

        std::string value;
        if (!readValue(args, i, inlineValue, value)) {
            runtime.hasArgumentError = true;
            warnings << "[args] missing value for " << key << "\n";
            continue;
        }

        if (key == "--config") {
            runtime.configPath = value;
            continue;
        }
        if (key == "--particle-count") {
            std::uint32_t v = config.particleCount;
            if (parseUint(value, v) && v >= 2u) {
                config.particleCount = v;
            } else {
                warnings << "[args] invalid --particle-count: " << value << "\n";
            }
            continue;
        }
        if (key == "--dt") {
            float v = config.dt;
            if (parseFloat(value, v) && v > 0.0f) {
                config.dt = v;
            } else {
                warnings << "[args] invalid --dt: " << value << "\n";
            }
            continue;
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
            continue;
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
            continue;
        }
        if (key == "--octree-theta") {
            float v = config.octreeTheta;
            if (parseFloat(value, v) && v > 0.01f) {
                config.octreeTheta = v;
            } else {
                warnings << "[args] invalid --octree-theta: " << value << "\n";
            }
            continue;
        }
        if (key == "--octree-softening") {
            float v = config.octreeSoftening;
            if (parseFloat(value, v) && v > 0.000001f) {
                config.octreeSoftening = v;
            } else {
                warnings << "[args] invalid --octree-softening: " << value << "\n";
            }
            continue;
        }
        if (key == "--frontend-particle-cap") {
            std::uint32_t v = config.frontendParticleCap;
            if (parseUint(value, v) && v >= 2u)
                config.frontendParticleCap = v;
            else
                warnings << "[args] invalid --frontend-particle-cap: " << value << "\n";
            continue;
        }
        if (key == "--zoom") {
            float v = config.defaultZoom;
            if (parseFloat(value, v) && v > 0.01f)
                config.defaultZoom = v;
            else
                warnings << "[args] invalid --zoom: " << value << "\n";
            continue;
        }
        if (key == "--luminosity") {
            int v = config.defaultLuminosity;
            if (parseInt(value, v) && v >= 0 && v <= 255)
                config.defaultLuminosity = v;
            else
                warnings << "[args] invalid --luminosity: " << value << "\n";
            continue;
        }
        if (key == "--ui-fps") {
            std::uint32_t v = config.uiFpsLimit;
            if (parseUint(value, v) && v >= 1u) {
                config.uiFpsLimit = v;
            } else
                warnings << "[args] invalid --ui-fps: " << value << "\n";
            continue;
        }
        if (key == "--backend-timeout-ms") {
            std::uint32_t v = config.frontendRemoteCommandTimeoutMs;
            if (parseUint(value, v) && v >= 10u && v <= 60000u) {
                config.frontendRemoteCommandTimeoutMs = v;
                config.frontendRemoteStatusTimeoutMs = v;
                config.frontendRemoteSnapshotTimeoutMs = v;
            } else {
                warnings << "[args] invalid --backend-timeout-ms: " << value << "\n";
            }
            continue;
        }
        if (key == "--backend-command-timeout-ms") {
            std::uint32_t v = config.frontendRemoteCommandTimeoutMs;
            if (parseUint(value, v) && v >= 10u && v <= 60000u) {
                config.frontendRemoteCommandTimeoutMs = v;
            } else {
                warnings << "[args] invalid --backend-command-timeout-ms: " << value << "\n";
            }
            continue;
        }
        if (key == "--backend-status-timeout-ms") {
            std::uint32_t v = config.frontendRemoteStatusTimeoutMs;
            if (parseUint(value, v) && v >= 10u && v <= 60000u) {
                config.frontendRemoteStatusTimeoutMs = v;
            } else {
                warnings << "[args] invalid --backend-status-timeout-ms: " << value << "\n";
            }
            continue;
        }
        if (key == "--backend-snapshot-timeout-ms") {
            std::uint32_t v = config.frontendRemoteSnapshotTimeoutMs;
            if (parseUint(value, v) && v >= 10u && v <= 60000u) {
                config.frontendRemoteSnapshotTimeoutMs = v;
            } else {
                warnings << "[args] invalid --backend-snapshot-timeout-ms: " << value << "\n";
            }
            continue;
        }
        if (key == "--export-directory") {
            config.exportDirectory = value;
            continue;
        }
        if (key == "--export-format") {
            config.exportFormat = value;
            continue;
        }
        if (key == "--input-file") {
            config.inputFile = value;
            continue;
        }
        if (key == "--input-format") {
            config.inputFormat = value;
            continue;
        }
        if (key == "--init-config-style") {
            config.initConfigStyle = value;
            continue;
        }
        if (key == "--preset-structure") {
            config.presetStructure = value;
            continue;
        }
        if (key == "--structure") {
            config.presetStructure = value;
            continue;
        }
        if (key == "--preset-size") {
            float v = config.presetSize;
            if (parseFloat(value, v) && v > 0.01f) {
                config.presetSize = v;
            } else {
                warnings << "[args] invalid --preset-size: " << value << "\n";
            }
            continue;
        }
        if (key == "--size") {
            float v = config.presetSize;
            if (parseFloat(value, v) && v > 0.01f) {
                config.presetSize = v;
            } else {
                warnings << "[args] invalid --size: " << value << "\n";
            }
            continue;
        }
        if (key == "--velocity-temperature") {
            float v = config.velocityTemperature;
            if (parseFloat(value, v) && v >= 0.0f) {
                config.velocityTemperature = v;
            } else {
                warnings << "[args] invalid --velocity-temperature: " << value << "\n";
            }
            continue;
        }
        if (key == "--particle-temperature") {
            float v = config.particleTemperature;
            if (parseFloat(value, v) && v >= 0.0f) {
                config.particleTemperature = v;
            } else {
                warnings << "[args] invalid --particle-temperature: " << value << "\n";
            }
            continue;
        }
        if (key == "--thermal-ambient") {
            float v = config.thermalAmbientTemperature;
            if (parseFloat(value, v) && v >= 0.0f) {
                config.thermalAmbientTemperature = v;
            } else {
                warnings << "[args] invalid --thermal-ambient: " << value << "\n";
            }
            continue;
        }
        if (key == "--thermal-specific-heat") {
            float v = config.thermalSpecificHeat;
            if (parseFloat(value, v) && v > 0.0f) {
                config.thermalSpecificHeat = v;
            } else {
                warnings << "[args] invalid --thermal-specific-heat: " << value << "\n";
            }
            continue;
        }
        if (key == "--thermal-heating") {
            float v = config.thermalHeatingCoeff;
            if (parseFloat(value, v) && v >= 0.0f) {
                config.thermalHeatingCoeff = v;
            } else {
                warnings << "[args] invalid --thermal-heating: " << value << "\n";
            }
            continue;
        }
        if (key == "--thermal-radiation") {
            float v = config.thermalRadiationCoeff;
            if (parseFloat(value, v) && v >= 0.0f) {
                config.thermalRadiationCoeff = v;
            } else {
                warnings << "[args] invalid --thermal-radiation: " << value << "\n";
            }
            continue;
        }
        if (key == "--init-mode") {
            config.initMode = value;
            continue;
        }
        if (key == "--init-seed") {
            std::uint32_t v = config.initSeed;
            if (parseUint(value, v)) {
                config.initSeed = v;
            } else {
                warnings << "[args] invalid --init-seed: " << value << "\n";
            }
            continue;
        }
        if (key == "--init-include-central-body") {
            bool parsedValue = config.initIncludeCentralBody;
            if (parseBool(value, parsedValue)) {
                config.initIncludeCentralBody = parsedValue;
            } else {
                warnings << "[args] invalid --init-include-central-body: " << value << "\n";
            }
            continue;
        }
        if (key == "--init-central-mass") {
            float v = config.initCentralMass;
            if (parseFloat(value, v) && v > 0.0f) {
                config.initCentralMass = v;
            } else {
                warnings << "[args] invalid --init-central-mass: " << value << "\n";
            }
            continue;
        }
        if (key == "--init-central-x") {
            float v = config.initCentralX;
            if (parseFloat(value, v)) {
                config.initCentralX = v;
            } else {
                warnings << "[args] invalid --init-central-x: " << value << "\n";
            }
            continue;
        }
        if (key == "--init-central-y") {
            float v = config.initCentralY;
            if (parseFloat(value, v)) {
                config.initCentralY = v;
            } else {
                warnings << "[args] invalid --init-central-y: " << value << "\n";
            }
            continue;
        }
        if (key == "--init-central-z") {
            float v = config.initCentralZ;
            if (parseFloat(value, v)) {
                config.initCentralZ = v;
            } else {
                warnings << "[args] invalid --init-central-z: " << value << "\n";
            }
            continue;
        }
        if (key == "--init-central-vx") {
            float v = config.initCentralVx;
            if (parseFloat(value, v)) {
                config.initCentralVx = v;
            } else {
                warnings << "[args] invalid --init-central-vx: " << value << "\n";
            }
            continue;
        }
        if (key == "--init-central-vy") {
            float v = config.initCentralVy;
            if (parseFloat(value, v)) {
                config.initCentralVy = v;
            } else {
                warnings << "[args] invalid --init-central-vy: " << value << "\n";
            }
            continue;
        }
        if (key == "--init-central-vz") {
            float v = config.initCentralVz;
            if (parseFloat(value, v)) {
                config.initCentralVz = v;
            } else {
                warnings << "[args] invalid --init-central-vz: " << value << "\n";
            }
            continue;
        }
        if (key == "--init-disk-mass") {
            float v = config.initDiskMass;
            if (parseFloat(value, v) && v > 0.0f) {
                config.initDiskMass = v;
            } else {
                warnings << "[args] invalid --init-disk-mass: " << value << "\n";
            }
            continue;
        }
        if (key == "--init-disk-radius-min") {
            float v = config.initDiskRadiusMin;
            if (parseFloat(value, v) && v > 0.0f) {
                config.initDiskRadiusMin = v;
            } else {
                warnings << "[args] invalid --init-disk-radius-min: " << value << "\n";
            }
            continue;
        }
        if (key == "--init-disk-radius-max") {
            float v = config.initDiskRadiusMax;
            if (parseFloat(value, v) && v > 0.0f) {
                config.initDiskRadiusMax = v;
            } else {
                warnings << "[args] invalid --init-disk-radius-max: " << value << "\n";
            }
            continue;
        }
        if (key == "--init-disk-thickness") {
            float v = config.initDiskThickness;
            if (parseFloat(value, v) && v >= 0.0f) {
                config.initDiskThickness = v;
            } else {
                warnings << "[args] invalid --init-disk-thickness: " << value << "\n";
            }
            continue;
        }
        if (key == "--init-velocity-scale") {
            float v = config.initVelocityScale;
            if (parseFloat(value, v) && v >= 0.0f) {
                config.initVelocityScale = v;
            } else {
                warnings << "[args] invalid --init-velocity-scale: " << value << "\n";
            }
            continue;
        }
        if (key == "--init-cloud-half-extent") {
            float v = config.initCloudHalfExtent;
            if (parseFloat(value, v) && v > 0.0f) {
                config.initCloudHalfExtent = v;
            } else {
                warnings << "[args] invalid --init-cloud-half-extent: " << value << "\n";
            }
            continue;
        }
        if (key == "--init-cloud-speed") {
            float v = config.initCloudSpeed;
            if (parseFloat(value, v) && v >= 0.0f) {
                config.initCloudSpeed = v;
            } else {
                warnings << "[args] invalid --init-cloud-speed: " << value << "\n";
            }
            continue;
        }
        if (key == "--init-particle-mass") {
            float v = config.initParticleMass;
            if (parseFloat(value, v) && v > 0.0f) {
                config.initParticleMass = v;
            } else {
                warnings << "[args] invalid --init-particle-mass: " << value << "\n";
            }
            continue;
        }
        if (key == "--sph") {
            bool parsedValue = config.sphEnabled;
            if (parseBool(value, parsedValue)) {
                config.sphEnabled = parsedValue;
            } else {
                warnings << "[args] invalid --sph: " << value << "\n";
            }
            continue;
        }
        if (key == "--sph-h") {
            float v = config.sphSmoothingLength;
            if (parseFloat(value, v) && v > 0.05f) {
                config.sphSmoothingLength = v;
            } else {
                warnings << "[args] invalid --sph-h: " << value << "\n";
            }
            continue;
        }
        if (key == "--sph-rest-density") {
            float v = config.sphRestDensity;
            if (parseFloat(value, v) && v > 0.01f) {
                config.sphRestDensity = v;
            } else {
                warnings << "[args] invalid --sph-rest-density: " << value << "\n";
            }
            continue;
        }
        if (key == "--sph-gas-constant") {
            float v = config.sphGasConstant;
            if (parseFloat(value, v) && v > 0.01f) {
                config.sphGasConstant = v;
            } else {
                warnings << "[args] invalid --sph-gas-constant: " << value << "\n";
            }
            continue;
        }
        if (key == "--sph-viscosity") {
            float v = config.sphViscosity;
            if (parseFloat(value, v) && v >= 0.0f) {
                config.sphViscosity = v;
            } else {
                warnings << "[args] invalid --sph-viscosity: " << value << "\n";
            }
            continue;
        }
        if (key == "--energy-every") {
            std::uint32_t v = config.energyMeasureEverySteps;
            if (parseUint(value, v) && v >= 1u) {
                config.energyMeasureEverySteps = v;
            } else {
                warnings << "[args] invalid --energy-every: " << value << "\n";
            }
            continue;
        }
        if (key == "--energy-sample-limit") {
            std::uint32_t v = config.energySampleLimit;
            if (parseUint(value, v) && v >= 64u) {
                config.energySampleLimit = v;
            } else {
                warnings << "[args] invalid --energy-sample-limit: " << value << "\n";
            }
            continue;
        }
        if (key == "--target-steps") {
            int v = runtime.targetSteps;
            if (parseInt(value, v) && v > 0) {
                runtime.targetSteps = v;
            } else {
                warnings << "[args] invalid --target-steps: " << value << "\n";
            }
            continue;
        }

        runtime.hasArgumentError = true;
        warnings << "[args] unknown option: " << key << "\n";
    }
}

void printUsage(std::ostream &out, std::string_view programName, bool headlessMode)
{
    out << "Usage: " << programName << " [options]\n";
    out << "Common options:\n";
    out << "  --config <path>\n";
    out << "  --particle-count <n>\n";
    out << "  --dt <float>\n";
    out << "  --solver <pairwise_cuda|octree_gpu|octree_cpu>\n";
    out << "  --integrator <euler|rk4>\n";
    out << "  --octree-theta <float>\n";
    out << "  --octree-softening <float>\n";
    out << "  --frontend-particle-cap <n>\n";
    out << "  --zoom <float>\n";
    out << "  --luminosity <0..255>\n";
    out << "  --ui-fps <n>\n";
    out << "  --backend-timeout-ms <10..60000>\n";
    out << "  --backend-command-timeout-ms <10..60000>\n";
    out << "  --backend-status-timeout-ms <10..60000>\n";
    out << "  --backend-snapshot-timeout-ms <10..60000>\n";
    out << "  --export-directory <path>\n";
    out << "  --export-format <vtk|vtk_binary|xyz|bin>\n";
    out << "  --input-file <path>\n";
    out << "  --input-format <auto|vtk|vtk_binary|xyz|bin>\n";
    out << "  --init-config-style <preset|detailed>\n";
    out << "  --preset-structure <disk_orbit|random_cloud|file>\n";
    out << "  --structure <disk_orbit|random_cloud|file> (alias)\n";
    out << "  --preset-size <float>\n";
    out << "  --size <float> (alias)\n";
    out << "  --velocity-temperature <float>\n";
    out << "  --particle-temperature <float>\n";
    out << "  --thermal-ambient <float>\n";
    out << "  --thermal-specific-heat <float>\n";
    out << "  --thermal-heating <float>\n";
    out << "  --thermal-radiation <float>\n";
    out << "  --init-mode <disk_orbit|random_cloud|file>\n";
    out << "  --init-seed <n>\n";
    out << "  --init-include-central-body <true|false>\n";
    out << "  --init-central-mass <float>\n";
    out << "  --init-central-x <float>\n";
    out << "  --init-central-y <float>\n";
    out << "  --init-central-z <float>\n";
    out << "  --init-central-vx <float>\n";
    out << "  --init-central-vy <float>\n";
    out << "  --init-central-vz <float>\n";
    out << "  --init-disk-mass <float>\n";
    out << "  --init-disk-radius-min <float>\n";
    out << "  --init-disk-radius-max <float>\n";
    out << "  --init-disk-thickness <float>\n";
    out << "  --init-velocity-scale <float>\n";
    out << "  --init-cloud-half-extent <float>\n";
    out << "  --init-cloud-speed <float>\n";
    out << "  --init-particle-mass <float>\n";
    out << "  --sph <true|false>\n";
    out << "  --sph-h <float>\n";
    out << "  --sph-rest-density <float>\n";
    out << "  --sph-gas-constant <float>\n";
    out << "  --sph-viscosity <float>\n";
    out << "  --energy-every <n>\n";
    out << "  --energy-sample-limit <n>\n";
    if (headlessMode) {
        out << "  --target-steps <n>\n";
        out << "  --export-on-exit <true|false>\n";
        out << "  --no-export-on-exit\n";
        out << "  env: GRAVITY_AUTO_SOLVER_FALLBACK=1 to auto-switch pairwise->octree_gpu for huge N\n";
    }
    out << "  --save-config\n";
    out << "  --help\n";
}
