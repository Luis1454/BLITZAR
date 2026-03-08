#include "config/SimulationOptionRegistry.hpp"

#include "config/SimulationArgsParse.hpp"
#include "config/SimulationModes.hpp"
#include "protocol/BackendProtocol.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>
namespace grav_config {
enum class OptionKind {
    Uint,
    Int,
    Float,
    Bool,
    String,
    Solver,
    Integrator,
    FrontendParticleCap,
    TimeoutTriple,
};
struct SimulationOptionEntry {
    SimulationOptionGroup group;
    OptionKind kind;
    std::string_view cliName;
    std::string_view cliAlias;
    std::string_view iniName;
    std::string_view iniAlias;
    std::string_view envName;
    std::string_view usage;
    std::string_view aliasUsage;
    std::ptrdiff_t offset;
    double minValue;
    double maxValue;
    bool hasMin;
    bool hasMax;
};
template <typename ValueType>
ValueType &memberAt(SimulationConfig &config, std::ptrdiff_t offset)
{
    return *reinterpret_cast<ValueType *>(reinterpret_cast<char *>(&config) + offset);
}
void emitInvalid(std::ostream &warnings, std::string_view source, std::string_view optionName, const std::string &value)
{
    warnings << source << " invalid " << optionName << ": " << value << "\n";
}
void emitClamped(
    std::ostream &warnings,
    std::string_view source,
    std::string_view optionName,
    std::uint32_t requested,
    std::uint32_t clamped)
{
    warnings << source << ' ' << optionName << " clamped to supported range [2, "
             << grav_protocol::kSnapshotMaxPoints << "]: "
             << requested << " -> " << clamped << "\n";
}
std::uint32_t clampFrontendParticleCap(std::uint32_t requested)
{
    if (requested > grav_protocol::kSnapshotMaxPoints) {
        return grav_protocol::kSnapshotMaxPoints;
    }
    return requested < 2u ? 2u : requested;
}
bool matchesCli(const SimulationOptionEntry &entry, const std::string &key, SimulationOptionGroup group)
{
    if (entry.group != group) {
        return false;
    }
    return key == entry.cliName || (!entry.cliAlias.empty() && key == entry.cliAlias);
}
bool matchesIni(const SimulationOptionEntry &entry, const std::string &key)
{
    return key == entry.iniName || (!entry.iniAlias.empty() && key == entry.iniAlias);
}
bool matchesEnv(const SimulationOptionEntry &entry, const std::string &key)
{
    return !entry.envName.empty() && key == entry.envName;
}
bool applyEntry(
    const SimulationOptionEntry &entry,
    const std::string &value,
    SimulationConfig &config,
    std::ostream &warnings,
    std::string_view source,
    std::string_view optionName)
{
    switch (entry.kind) {
        case OptionKind::Uint: {
            std::uint32_t parsed = memberAt<std::uint32_t>(config, entry.offset);
            if (!SimulationArgsParse::parseUint(value, parsed)
                || (entry.hasMin && parsed < static_cast<std::uint32_t>(entry.minValue))
                || (entry.hasMax && parsed > static_cast<std::uint32_t>(entry.maxValue))) {
                emitInvalid(warnings, source, optionName, value);
                return true;
            }
            memberAt<std::uint32_t>(config, entry.offset) = parsed;
            return true;
        }
        case OptionKind::Int: {
            int parsed = memberAt<int>(config, entry.offset);
            if (!SimulationArgsParse::parseInt(value, parsed)
                || (entry.hasMin && parsed < static_cast<int>(entry.minValue))
                || (entry.hasMax && parsed > static_cast<int>(entry.maxValue))) {
                emitInvalid(warnings, source, optionName, value);
                return true;
            }
            memberAt<int>(config, entry.offset) = parsed;
            return true;
        }
        case OptionKind::Float: {
            float parsed = memberAt<float>(config, entry.offset);
            if (!SimulationArgsParse::parseFloat(value, parsed)
                || (entry.hasMin && parsed < static_cast<float>(entry.minValue))
                || (entry.hasMax && parsed > static_cast<float>(entry.maxValue))) {
                emitInvalid(warnings, source, optionName, value);
                return true;
            }
            memberAt<float>(config, entry.offset) = parsed;
            return true;
        }
        case OptionKind::Bool: {
            bool parsed = memberAt<bool>(config, entry.offset);
            if (!SimulationArgsParse::parseBool(value, parsed)) {
                emitInvalid(warnings, source, optionName, value);
                return true;
            }
            memberAt<bool>(config, entry.offset) = parsed;
            return true;
        }
        case OptionKind::String:
            memberAt<std::string>(config, entry.offset) = value;
            return true;
        case OptionKind::Solver: {
            std::string canonical;
            if (!grav_modes::normalizeSolver(value, canonical)) {
                warnings << source << " invalid " << optionName
                         << ": " << value << " (allowed: pairwise_cuda|octree_gpu|octree_cpu)\n";
                return true;
            }
            memberAt<std::string>(config, entry.offset) = canonical;
            return true;
        }
        case OptionKind::Integrator: {
            std::string canonical;
            if (!grav_modes::normalizeIntegrator(value, canonical)) {
                warnings << source << " invalid " << optionName
                         << ": " << value << " (allowed: euler|rk4)\n";
                return true;
            }
            memberAt<std::string>(config, entry.offset) = canonical;
            return true;
        }
        case OptionKind::FrontendParticleCap: {
            std::uint32_t parsed = memberAt<std::uint32_t>(config, entry.offset);
            if (!SimulationArgsParse::parseUint(value, parsed) || parsed < 2u) {
                emitInvalid(warnings, source, optionName, value);
                return true;
            }
            const std::uint32_t clamped = clampFrontendParticleCap(parsed);
            memberAt<std::uint32_t>(config, entry.offset) = clamped;
            if (clamped != parsed) {
                emitClamped(warnings, source, optionName, parsed, clamped);
            }
            return true;
        }
        case OptionKind::TimeoutTriple: {
            std::uint32_t parsed = config.frontendRemoteCommandTimeoutMs;
            if (!SimulationArgsParse::parseUint(value, parsed) || parsed < 10u || parsed > 60000u) {
                emitInvalid(warnings, source, optionName, value);
                return true;
            }
            config.frontendRemoteCommandTimeoutMs = parsed;
            config.frontendRemoteStatusTimeoutMs = parsed;
            config.frontendRemoteSnapshotTimeoutMs = parsed;
            return true;
        }
    }
    return false;
}

const SimulationOptionEntry kSimulationOptions[] = {
    {SimulationOptionGroup::Core, OptionKind::Uint, "--particle-count", "", "particle_count", "", "GRAVITY_BACKEND_PARTICLES", "  --particle-count <n>\n", "", offsetof(SimulationConfig, particleCount), 2.0, 0.0, true, false},
    {SimulationOptionGroup::Core, OptionKind::Float, "--dt", "", "dt", "", "", "  --dt <float>\n", "", offsetof(SimulationConfig, dt), 0.000001, 0.0, true, false},
    {SimulationOptionGroup::Core, OptionKind::Solver, "--solver", "", "solver", "", "", "  --solver <pairwise_cuda|octree_gpu|octree_cpu>\n", "", offsetof(SimulationConfig, solver), 0.0, 0.0, false, false},
    {SimulationOptionGroup::Core, OptionKind::Integrator, "--integrator", "", "integrator", "", "", "  --integrator <euler|rk4>\n", "", offsetof(SimulationConfig, integrator), 0.0, 0.0, false, false},
    {SimulationOptionGroup::Core, OptionKind::Float, "--octree-theta", "", "octree_theta", "", "", "  --octree-theta <float>\n", "", offsetof(SimulationConfig, octreeTheta), 0.01, 0.0, true, false},
    {SimulationOptionGroup::Core, OptionKind::Float, "--octree-softening", "", "octree_softening", "", "", "  --octree-softening <float>\n", "", offsetof(SimulationConfig, octreeSoftening), 0.000001, 0.0, true, false},
    {SimulationOptionGroup::Core, OptionKind::FrontendParticleCap, "--frontend-particle-cap", "", "frontend_particle_cap", "", "GRAVITY_FRONTEND_DRAW_CAP", "  --frontend-particle-cap <n>\n", "", offsetof(SimulationConfig, frontendParticleCap), 0.0, 0.0, false, false},
    {SimulationOptionGroup::Frontend, OptionKind::Float, "--zoom", "", "default_zoom", "", "", "  --zoom <float>\n", "", offsetof(SimulationConfig, defaultZoom), 0.01, 0.0, true, false},
    {SimulationOptionGroup::Frontend, OptionKind::Int, "--luminosity", "", "default_luminosity", "", "", "  --luminosity <0..255>\n", "", offsetof(SimulationConfig, defaultLuminosity), 0.0, 255.0, true, true},
    {SimulationOptionGroup::Frontend, OptionKind::Uint, "--ui-fps", "", "ui_fps_limit", "", "", "  --ui-fps <n>\n", "", offsetof(SimulationConfig, uiFpsLimit), 1.0, 0.0, true, false},
    {SimulationOptionGroup::Frontend, OptionKind::TimeoutTriple, "--backend-timeout-ms", "", "frontend_remote_timeout_ms", "", "", "  --backend-timeout-ms <10..60000>\n", "", 0, 10.0, 60000.0, true, true},
    {SimulationOptionGroup::Frontend, OptionKind::Uint, "--backend-command-timeout-ms", "", "frontend_remote_command_timeout_ms", "", "", "  --backend-command-timeout-ms <10..60000>\n", "", offsetof(SimulationConfig, frontendRemoteCommandTimeoutMs), 10.0, 60000.0, true, true},
    {SimulationOptionGroup::Frontend, OptionKind::Uint, "--backend-status-timeout-ms", "", "frontend_remote_status_timeout_ms", "", "", "  --backend-status-timeout-ms <10..60000>\n", "", offsetof(SimulationConfig, frontendRemoteStatusTimeoutMs), 10.0, 60000.0, true, true},
    {SimulationOptionGroup::Frontend, OptionKind::Uint, "--backend-snapshot-timeout-ms", "", "frontend_remote_snapshot_timeout_ms", "", "", "  --backend-snapshot-timeout-ms <10..60000>\n", "", offsetof(SimulationConfig, frontendRemoteSnapshotTimeoutMs), 10.0, 60000.0, true, true},
    {SimulationOptionGroup::Frontend, OptionKind::String, "--export-directory", "", "export_directory", "", "", "  --export-directory <path>\n", "", offsetof(SimulationConfig, exportDirectory), 0.0, 0.0, false, false},
    {SimulationOptionGroup::Frontend, OptionKind::String, "--export-format", "", "export_format", "", "", "  --export-format <vtk|vtk_binary|xyz|bin>\n", "", offsetof(SimulationConfig, exportFormat), 0.0, 0.0, false, false},
    {SimulationOptionGroup::Frontend, OptionKind::String, "--input-file", "", "input_file", "", "", "  --input-file <path>\n", "", offsetof(SimulationConfig, inputFile), 0.0, 0.0, false, false},
    {SimulationOptionGroup::Frontend, OptionKind::String, "--input-format", "", "input_format", "", "", "  --input-format <auto|vtk|vtk_binary|xyz|bin>\n", "", offsetof(SimulationConfig, inputFormat), 0.0, 0.0, false, false},
    {SimulationOptionGroup::Frontend, OptionKind::String, "--init-config-style", "", "init_config_style", "", "", "  --init-config-style <preset|detailed>\n", "", offsetof(SimulationConfig, initConfigStyle), 0.0, 0.0, false, false},
    {SimulationOptionGroup::Frontend, OptionKind::String, "--preset-structure", "--structure", "preset_structure", "", "", "  --preset-structure <disk_orbit|random_cloud|file>\n", "  --structure <disk_orbit|random_cloud|file> (alias)\n", offsetof(SimulationConfig, presetStructure), 0.0, 0.0, false, false},
    {SimulationOptionGroup::Frontend, OptionKind::Float, "--preset-size", "--size", "preset_size", "", "", "  --preset-size <float>\n", "  --size <float> (alias)\n", offsetof(SimulationConfig, presetSize), 0.01, 0.0, true, false},
    {SimulationOptionGroup::Frontend, OptionKind::Float, "--velocity-temperature", "", "velocity_temperature", "temperature", "", "  --velocity-temperature <float>\n", "", offsetof(SimulationConfig, velocityTemperature), 0.0, 0.0, true, false},
    {SimulationOptionGroup::Frontend, OptionKind::Float, "--particle-temperature", "", "particle_temperature", "", "", "  --particle-temperature <float>\n", "", offsetof(SimulationConfig, particleTemperature), 0.0, 0.0, true, false},
    {SimulationOptionGroup::Frontend, OptionKind::Float, "--thermal-ambient", "", "thermal_ambient_temperature", "", "", "  --thermal-ambient <float>\n", "", offsetof(SimulationConfig, thermalAmbientTemperature), 0.0, 0.0, true, false},
    {SimulationOptionGroup::Frontend, OptionKind::Float, "--thermal-specific-heat", "", "thermal_specific_heat", "", "", "  --thermal-specific-heat <float>\n", "", offsetof(SimulationConfig, thermalSpecificHeat), 0.000001, 0.0, true, false},
    {SimulationOptionGroup::Frontend, OptionKind::Float, "--thermal-heating", "", "thermal_heating_coeff", "", "", "  --thermal-heating <float>\n", "", offsetof(SimulationConfig, thermalHeatingCoeff), 0.0, 0.0, true, false},
    {SimulationOptionGroup::Frontend, OptionKind::Float, "--thermal-radiation", "", "thermal_radiation_coeff", "", "", "  --thermal-radiation <float>\n", "", offsetof(SimulationConfig, thermalRadiationCoeff), 0.0, 0.0, true, false},
    {SimulationOptionGroup::InitState, OptionKind::String, "--init-mode", "", "init_mode", "", "", "  --init-mode <disk_orbit|random_cloud|file>\n", "", offsetof(SimulationConfig, initMode), 0.0, 0.0, false, false},
    {SimulationOptionGroup::InitState, OptionKind::Uint, "--init-seed", "", "init_seed", "", "", "  --init-seed <n>\n", "", offsetof(SimulationConfig, initSeed), 0.0, 0.0, false, false},
    {SimulationOptionGroup::InitState, OptionKind::Bool, "--init-include-central-body", "", "init_include_central_body", "", "", "  --init-include-central-body <true|false>\n", "", offsetof(SimulationConfig, initIncludeCentralBody), 0.0, 0.0, false, false},
    {SimulationOptionGroup::InitState, OptionKind::Float, "--init-central-mass", "", "init_central_mass", "", "", "  --init-central-mass <float>\n", "", offsetof(SimulationConfig, initCentralMass), 0.000001, 0.0, true, false},
    {SimulationOptionGroup::InitState, OptionKind::Float, "--init-central-x", "", "init_central_x", "", "", "  --init-central-x <float>\n", "", offsetof(SimulationConfig, initCentralX), 0.0, 0.0, false, false},
    {SimulationOptionGroup::InitState, OptionKind::Float, "--init-central-y", "", "init_central_y", "", "", "  --init-central-y <float>\n", "", offsetof(SimulationConfig, initCentralY), 0.0, 0.0, false, false},
    {SimulationOptionGroup::InitState, OptionKind::Float, "--init-central-z", "", "init_central_z", "", "", "  --init-central-z <float>\n", "", offsetof(SimulationConfig, initCentralZ), 0.0, 0.0, false, false},
    {SimulationOptionGroup::InitState, OptionKind::Float, "--init-central-vx", "", "init_central_vx", "", "", "  --init-central-vx <float>\n", "", offsetof(SimulationConfig, initCentralVx), 0.0, 0.0, false, false},
    {SimulationOptionGroup::InitState, OptionKind::Float, "--init-central-vy", "", "init_central_vy", "", "", "  --init-central-vy <float>\n", "", offsetof(SimulationConfig, initCentralVy), 0.0, 0.0, false, false},
    {SimulationOptionGroup::InitState, OptionKind::Float, "--init-central-vz", "", "init_central_vz", "", "", "  --init-central-vz <float>\n", "", offsetof(SimulationConfig, initCentralVz), 0.0, 0.0, false, false},
    {SimulationOptionGroup::InitState, OptionKind::Float, "--init-disk-mass", "", "init_disk_mass", "", "", "  --init-disk-mass <float>\n", "", offsetof(SimulationConfig, initDiskMass), 0.000001, 0.0, true, false},
    {SimulationOptionGroup::InitState, OptionKind::Float, "--init-disk-radius-min", "", "init_disk_radius_min", "", "", "  --init-disk-radius-min <float>\n", "", offsetof(SimulationConfig, initDiskRadiusMin), 0.000001, 0.0, true, false},
    {SimulationOptionGroup::InitState, OptionKind::Float, "--init-disk-radius-max", "", "init_disk_radius_max", "", "", "  --init-disk-radius-max <float>\n", "", offsetof(SimulationConfig, initDiskRadiusMax), 0.000001, 0.0, true, false},
    {SimulationOptionGroup::InitState, OptionKind::Float, "--init-disk-thickness", "", "init_disk_thickness", "", "", "  --init-disk-thickness <float>\n", "", offsetof(SimulationConfig, initDiskThickness), 0.0, 0.0, true, false},
    {SimulationOptionGroup::InitState, OptionKind::Float, "--init-velocity-scale", "", "init_velocity_scale", "", "", "  --init-velocity-scale <float>\n", "", offsetof(SimulationConfig, initVelocityScale), 0.0, 0.0, true, false},
    {SimulationOptionGroup::InitState, OptionKind::Float, "--init-cloud-half-extent", "", "init_cloud_half_extent", "", "", "  --init-cloud-half-extent <float>\n", "", offsetof(SimulationConfig, initCloudHalfExtent), 0.000001, 0.0, true, false},
    {SimulationOptionGroup::InitState, OptionKind::Float, "--init-cloud-speed", "", "init_cloud_speed", "", "", "  --init-cloud-speed <float>\n", "", offsetof(SimulationConfig, initCloudSpeed), 0.0, 0.0, true, false},
    {SimulationOptionGroup::InitState, OptionKind::Float, "--init-particle-mass", "", "init_particle_mass", "", "", "  --init-particle-mass <float>\n", "", offsetof(SimulationConfig, initParticleMass), 0.000001, 0.0, true, false},
    {SimulationOptionGroup::Fluid, OptionKind::Bool, "--sph", "", "sph_enabled", "", "", "  --sph <true|false>\n", "", offsetof(SimulationConfig, sphEnabled), 0.0, 0.0, false, false},
    {SimulationOptionGroup::Fluid, OptionKind::Float, "--sph-h", "", "sph_smoothing_length", "", "", "  --sph-h <float>\n", "", offsetof(SimulationConfig, sphSmoothingLength), 0.05, 0.0, true, false},
    {SimulationOptionGroup::Fluid, OptionKind::Float, "--sph-rest-density", "", "sph_rest_density", "", "", "  --sph-rest-density <float>\n", "", offsetof(SimulationConfig, sphRestDensity), 0.01, 0.0, true, false},
    {SimulationOptionGroup::Fluid, OptionKind::Float, "--sph-gas-constant", "", "sph_gas_constant", "", "", "  --sph-gas-constant <float>\n", "", offsetof(SimulationConfig, sphGasConstant), 0.01, 0.0, true, false},
    {SimulationOptionGroup::Fluid, OptionKind::Float, "--sph-viscosity", "", "sph_viscosity", "", "", "  --sph-viscosity <float>\n", "", offsetof(SimulationConfig, sphViscosity), 0.0, 0.0, true, false},
    {SimulationOptionGroup::Fluid, OptionKind::Uint, "--energy-every", "", "energy_measure_every_steps", "", "", "  --energy-every <n>\n", "", offsetof(SimulationConfig, energyMeasureEverySteps), 1.0, 0.0, true, false},
    {SimulationOptionGroup::Fluid, OptionKind::Uint, "--energy-sample-limit", "", "energy_sample_limit", "", "", "  --energy-sample-limit <n>\n", "", offsetof(SimulationConfig, energySampleLimit), 64.0, 0.0, true, false},
};

template <typename Matcher>
bool applyMatchingEntry(
    Matcher matcher,
    const std::string &key,
    const std::string &value,
    SimulationConfig &config,
    std::ostream &warnings,
    std::string_view source)
{
    for (const SimulationOptionEntry &entry : kSimulationOptions) {
        if (!matcher(entry, key)) {
            continue;
        }
        return applyEntry(entry, value, config, warnings, source, key);
    }
    return false;
}

bool applyCliOption(
    SimulationOptionGroup group,
    const std::string &key,
    const std::string &value,
    SimulationConfig &config,
    std::ostream &warnings)
{
    return applyMatchingEntry(
        [group](const SimulationOptionEntry &entry, const std::string &candidate) {
            return matchesCli(entry, candidate, group);
        },
        key,
        value,
        config,
        warnings,
        "[args]");
}

bool applyIniOption(const std::string &key, const std::string &value, SimulationConfig &config, std::ostream &warnings)
{
    return applyMatchingEntry(matchesIni, key, value, config, warnings, "[config]");
}

bool applyEnvOption(const std::string &key, const std::string &value, SimulationConfig &config, std::ostream &warnings)
{
    return applyMatchingEntry(matchesEnv, key, value, config, warnings, "[env]");
}

void printCliUsage(std::ostream &out, SimulationOptionGroup group)
{
    for (const SimulationOptionEntry &entry : kSimulationOptions) {
        if (entry.group != group || entry.usage.empty()) {
            continue;
        }
        out << entry.usage;
        if (!entry.aliasUsage.empty()) {
            out << entry.aliasUsage;
        }
    }
}

} // namespace grav_config
