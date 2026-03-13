#include "config/SimulationConfigDirectiveWrite.hpp"

#include "config/SimulationPerformanceProfile.hpp"
#include "protocol/ServerProtocol.hpp"

#include <algorithm>
#include <cctype>

namespace grav_config {

static std::string quoteDirectiveValue(const std::string &value)
{
    if (value.empty()) {
        return "\"\"";
    }
    const bool needsQuotes = std::any_of(value.begin(), value.end(), [](unsigned char c) {
        return std::isspace(c) != 0 || c == ',' || c == '(' || c == ')' || c == '"' || c == '\'';
    });
    if (!needsQuotes) {
        return value;
    }
    std::string quoted;
    quoted.reserve(value.size() + 2u);
    quoted.push_back('"');
    for (char c : value) {
        if (c == '"' || c == '\\') {
            quoted.push_back('\\');
        }
        quoted.push_back(c);
    }
    quoted.push_back('"');
    return quoted;
}

static bool matchesManagedPerformanceFields(const SimulationConfig &lhs, const SimulationConfig &rhs)
{
    return lhs.clientParticleCap == rhs.clientParticleCap
        && lhs.snapshotPublishPeriodMs == rhs.snapshotPublishPeriodMs
        && lhs.energyMeasureEverySteps == rhs.energyMeasureEverySteps
        && lhs.energySampleLimit == rhs.energySampleLimit
        && lhs.substepTargetDt == rhs.substepTargetDt
        && lhs.maxSubsteps == rhs.maxSubsteps;
}

void SimulationConfigDirective::write(std::ostream &out, const SimulationConfig &config)
{
    SimulationConfig profileReference = SimulationConfig::defaults();
    profileReference.performanceProfile = config.performanceProfile;
    applyPerformanceProfile(profileReference);
    const bool emitCustomProfile =
        config.performanceProfile == "custom" || !matchesManagedPerformanceFields(config, profileReference);
    const std::string effectiveProfile = emitCustomProfile ? "custom" : config.performanceProfile;

    out << "# ==================================================\n";
    out << "# A.S.T.E.R. directive config\n";
    out << "# Generated automatically. Edit values then restart.\n";
    out << "# ==================================================\n\n";

    out << "simulation(particle_count=" << config.particleCount
        << ", dt=" << config.dt
        << ", solver=" << config.solver
        << ", integrator=" << config.integrator << ")\n";
    out << "performance(profile=" << effectiveProfile;
    if (emitCustomProfile) {
        out << ", draw_cap=" << std::min<std::uint32_t>(grav_protocol::kSnapshotMaxPoints, std::max<std::uint32_t>(2u, config.clientParticleCap))
            << ", snapshot_ms=" << std::max<std::uint32_t>(1u, config.snapshotPublishPeriodMs)
            << ", energy_every=" << std::max<std::uint32_t>(1u, config.energyMeasureEverySteps)
            << ", sample_limit=" << std::max<std::uint32_t>(64u, config.energySampleLimit)
            << ", substep_target_dt=" << config.substepTargetDt
            << ", max_substeps=" << std::max<std::uint32_t>(1u, config.maxSubsteps);
    }
    out << ")\n";
    out << "octree(theta=" << config.octreeTheta
        << ", softening=" << config.octreeSoftening << ")\n";
    out << "client(zoom=" << config.defaultZoom
        << ", luminosity=" << config.defaultLuminosity
        << ", ui_fps=" << config.uiFpsLimit
        << ", command_timeout_ms=" << config.clientRemoteCommandTimeoutMs
        << ", status_timeout_ms=" << config.clientRemoteStatusTimeoutMs
        << ", snapshot_timeout_ms=" << config.clientRemoteSnapshotTimeoutMs << ")\n";
    out << "export(directory=" << quoteDirectiveValue(config.exportDirectory)
        << ", format=" << config.exportFormat << ")\n";
    out << "scene(style=" << config.initConfigStyle
        << ", preset=" << config.presetStructure
        << ", mode=" << config.initMode
        << ", file=" << quoteDirectiveValue(config.inputFile)
        << ", format=" << config.inputFormat << ")\n";
    out << "preset(size=" << config.presetSize
        << ", velocity_temperature=" << config.velocityTemperature
        << ", temperature=" << config.particleTemperature << ")\n";
    out << "thermal(ambient=" << config.thermalAmbientTemperature
        << ", specific_heat=" << config.thermalSpecificHeat
        << ", heating=" << config.thermalHeatingCoeff
        << ", radiation=" << config.thermalRadiationCoeff << ")\n";
    out << "generation(seed=" << config.initSeed
        << ", include_central_body=" << (config.initIncludeCentralBody ? "true" : "false") << ")\n";
    out << "central_body(mass=" << config.initCentralMass
        << ", x=" << config.initCentralX
        << ", y=" << config.initCentralY
        << ", z=" << config.initCentralZ
        << ", vx=" << config.initCentralVx
        << ", vy=" << config.initCentralVy
        << ", vz=" << config.initCentralVz << ")\n";
    out << "disk(mass=" << config.initDiskMass
        << ", radius_min=" << config.initDiskRadiusMin
        << ", radius_max=" << config.initDiskRadiusMax
        << ", thickness=" << config.initDiskThickness
        << ", velocity_scale=" << config.initVelocityScale << ")\n";
    out << "cloud(half_extent=" << config.initCloudHalfExtent
        << ", speed=" << config.initCloudSpeed
        << ", particle_mass=" << config.initParticleMass << ")\n";
    out << "sph(enabled=" << (config.sphEnabled ? "true" : "false")
        << ", smoothing_length=" << config.sphSmoothingLength
        << ", rest_density=" << config.sphRestDensity
        << ", gas_constant=" << config.sphGasConstant
        << ", viscosity=" << config.sphViscosity << ")\n";
}

} // namespace grav_config
