#include "config/SimulationConfig.hpp"
#include "config/SimulationOptionRegistry.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

static std::uint32_t clampFrontendParticleCap(std::uint32_t requested)
{
    if (requested > grav_protocol::kSnapshotMaxPoints) {
        return grav_protocol::kSnapshotMaxPoints;
    }
    return requested < 2u ? 2u : requested;
}

static std::string trim(const std::string &value)
{
    const auto begin = std::find_if_not(value.begin(), value.end(), [](unsigned char c) { return std::isspace(c) != 0; });
    const auto end = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char c) { return std::isspace(c) != 0; }).base();
    if (begin >= end) {
        return {};
    }
    return std::string(begin, end);
}

SimulationConfig SimulationConfig::defaults()
{
    return SimulationConfig{};
}

SimulationConfig SimulationConfig::loadOrCreate(const std::string &path)
{
    SimulationConfig config = defaults();
    std::ifstream in(path);
    if (!in.is_open()) {
        config.save(path);
        return config;
    }

    std::string line;
    while (std::getline(in, line)) {
        const std::string stripped = trim(line);
        if (stripped.empty() || stripped[0] == '#' || stripped[0] == ';') {
            continue;
        }
        const std::size_t eq = stripped.find('=');
        if (eq == std::string::npos) {
            continue;
        }
        const std::string key = trim(stripped.substr(0, eq));
        const std::string value = trim(stripped.substr(eq + 1));
        if (!grav_config::applyIniOption(key, value, config, std::cerr)) {
            std::cerr << "[config] unknown key ignored: " << key << "\n";
        }
    }
    return config;
}

bool SimulationConfig::save(const std::string &path) const
{
    std::filesystem::path fsPath(path);
    if (fsPath.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(fsPath.parent_path(), ec);
    }

    std::ofstream out(path, std::ios::trunc);
    if (!out.is_open()) {
        return false;
    }

    out << "# ==================================================\n";
    out << "# CUDA gravity simulation config\n";
    out << "# Generated automatically. Edit values then restart.\n";
    out << "# ==================================================\n";
    out << "\n";

    out << "# [Simulation]\n";
    out << "# Total number of particles simulated by backend.\n";
    out << "particle_count=" << particleCount << "\n";
    out << "# Fixed simulation timestep.\n";
    out << "dt=" << dt << "\n";
    out << "# Gravity solver: pairwise_cuda | octree_gpu | octree_cpu\n";
    out << "solver=" << solver << "\n";
    out << "# Time integrator: euler | rk4\n";
    out << "integrator=" << integrator << "\n";
    out << "\n";

    out << "# [Octree]\n";
    out << "# Barnes-Hut opening angle.\n";
    out << "octree_theta=" << octreeTheta << "\n";
    out << "# Distance softening used in octree force evaluation.\n";
    out << "octree_softening=" << octreeSoftening << "\n";
    out << "\n";

    out << "# [Frontend defaults]\n";
    out << "# Rendering cap for displayed particles.\n";
    out << "# Supported range [" << 2u
        << ", " << grav_protocol::kSnapshotMaxPoints << "].\n";
    out << "frontend_particle_cap=" << clampFrontendParticleCap(frontendParticleCap) << "\n";
    out << "# Initial camera zoom.\n";
    out << "default_zoom=" << defaultZoom << "\n";
    out << "# Initial particle alpha [0..255].\n";
    out << "default_luminosity=" << defaultLuminosity << "\n";
    out << "# UI refresh target FPS.\n";
    out << "ui_fps_limit=" << uiFpsLimit << "\n";
    out << "# Remote backend socket timeouts (ms) for frontend client calls.\n";
    out << "frontend_remote_command_timeout_ms=" << frontendRemoteCommandTimeoutMs << "\n";
    out << "frontend_remote_status_timeout_ms=" << frontendRemoteStatusTimeoutMs << "\n";
    out << "frontend_remote_snapshot_timeout_ms=" << frontendRemoteSnapshotTimeoutMs << "\n";
    out << "\n";

    out << "# [Export]\n";
    out << "# Directory used for snapshot exports.\n";
    out << "export_directory=" << exportDirectory << "\n";
    out << "# Snapshot format: vtk | vtk_binary | xyz | bin\n";
    out << "export_format=" << exportFormat << "\n";
    out << "\n";

    out << "# [Initial state file import]\n";
    out << "# Leave empty to disable direct file import.\n";
    out << "input_file=" << inputFile << "\n";
    out << "# Input file format: auto | vtk | vtk_binary | xyz | bin\n";
    out << "input_format=" << inputFormat << "\n";
    out << "\n";

    out << "# [Quick preset initialization]\n";
    out << "# init_config_style: preset | detailed\n";
    out << "# preset mode uses only basic info (structure, size, velocity_temperature, particle_count).\n";
    out << "init_config_style=" << initConfigStyle << "\n";
    out << "# preset_structure: disk_orbit | random_cloud | file\n";
    out << "preset_structure=" << presetStructure << "\n";
    out << "# characteristic size of generated structure.\n";
    out << "preset_size=" << presetSize << "\n";
    out << "# random velocity agitation (unitless). 0 disables thermal noise.\n";
    out << "velocity_temperature=" << velocityTemperature << "\n";
    out << "# initial temperature assigned to generated particles.\n";
    out << "particle_temperature=" << particleTemperature << "\n";
    out << "# thermal model used during simulation.\n";
    out << "# NOTE: non-zero heating/radiation alter total energy evolution.\n";
    out << "# Keep both at 0 for near-conservative runs.\n";
    out << "# thermal_radiation_coeff acts as emissivity scale for Stefan-Boltzmann exchange.\n";
    out << "thermal_ambient_temperature=" << thermalAmbientTemperature << "\n";
    out << "thermal_specific_heat=" << thermalSpecificHeat << "\n";
    out << "thermal_heating_coeff=" << thermalHeatingCoeff << "\n";
    out << "thermal_radiation_coeff=" << thermalRadiationCoeff << "\n";
    out << "\n";

    out << "# [Initial state generation]\n";
    out << "# init_mode: disk_orbit | random_cloud | file\n";
    out << "# - file: tries input_file first\n";
    out << "# - disk_orbit/random_cloud: generated from init_ keys below\n";
    out << "# NOTE: used when init_config_style=detailed\n";
    out << "init_mode=" << initMode << "\n";
    out << "# RNG seed for generated states.\n";
    out << "init_seed=" << initSeed << "\n";
    out << "# Add central body before generated particles.\n";
    out << "init_include_central_body=" << (initIncludeCentralBody ? "true" : "false") << "\n";
    out << "# Central body mass and initial state.\n";
    out << "init_central_mass=" << initCentralMass << "\n";
    out << "init_central_x=" << initCentralX << "\n";
    out << "init_central_y=" << initCentralY << "\n";
    out << "init_central_z=" << initCentralZ << "\n";
    out << "init_central_vx=" << initCentralVx << "\n";
    out << "init_central_vy=" << initCentralVy << "\n";
    out << "init_central_vz=" << initCentralVz << "\n";
    out << "# disk_orbit parameters.\n";
    out << "init_disk_mass=" << initDiskMass << "\n";
    out << "init_disk_radius_min=" << initDiskRadiusMin << "\n";
    out << "init_disk_radius_max=" << initDiskRadiusMax << "\n";
    out << "init_disk_thickness=" << initDiskThickness << "\n";
    out << "init_velocity_scale=" << initVelocityScale << "\n";
    out << "# random_cloud parameters.\n";
    out << "init_cloud_half_extent=" << initCloudHalfExtent << "\n";
    out << "init_cloud_speed=" << initCloudSpeed << "\n";
    out << "init_particle_mass=" << initParticleMass << "\n";
    out << "\n";

    out << "# [SPH option]\n";
    out << "# SPH can be enabled independently of gravity solver.\n";
    out << "sph_enabled=" << (sphEnabled ? "true" : "false") << "\n";
    out << "sph_smoothing_length=" << sphSmoothingLength << "\n";
    out << "sph_rest_density=" << sphRestDensity << "\n";
    out << "sph_gas_constant=" << sphGasConstant << "\n";
    out << "sph_viscosity=" << sphViscosity << "\n";
    out << "\n";

    out << "# [Energy monitoring]\n";
    out << "# Compute energies every N steps.\n";
    out << "energy_measure_every_steps=" << energyMeasureEverySteps << "\n";
    out << "# Max particles used for sampled energy estimation.\n";
    out << "energy_sample_limit=" << energySampleLimit << "\n";
    return true;
}
