/*
 * @file engine/config/registry/CfgEntriesClient.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Responsibility-focused simulation option definitions.
 */

#include "CfgInternal.hpp"

#include "config/core/CfgConfig.hpp"

#include <cstddef>

namespace bltzr_config {

extern const SimulationOptionEntry kClientPrimaryOptions[] = {
    {SimulationOptionGroup::Client, OptionKind::Float, "--zoom", "", "default_zoom", "", "",
     "  --zoom <float>\n", "", offsetof(SimulationConfig, defaultZoom), 0.0, 0.0, true, false},
    {SimulationOptionGroup::Client, OptionKind::Int, "--luminosity", "", "default_luminosity", "",
     "", "  --luminosity <0..255>\n", "", offsetof(SimulationConfig, defaultLuminosity),
     kLuminosityMin, kLuminosityMax, true, true},
    {SimulationOptionGroup::Client, OptionKind::String, "--ui-theme", "", "ui_theme", "", "",
     "  --ui-theme <light|dark>\n", "", offsetof(SimulationConfig, uiTheme), 0.0, 0.0, false,
     false},
    {SimulationOptionGroup::Client, OptionKind::Uint, "--ui-fps", "", "ui_fps_limit", "", "",
     "  --ui-fps <n>\n", "", offsetof(SimulationConfig, uiFpsLimit), 1.0, 0.0, true, false},
    {SimulationOptionGroup::Client, OptionKind::TimeoutTriple, "--server-timeout-ms", "",
     "client_remote_timeout_ms", "", "", "  --server-timeout-ms <10..60000>\n", "", 0,
     kRuntimeRemoteTimeoutMinMs, kRuntimeRemoteTimeoutMaxMs, true, true},
    {SimulationOptionGroup::Client, OptionKind::Uint, "--server-command-timeout-ms", "",
     "client_remote_command_timeout_ms", "", "", "  --server-command-timeout-ms <10..60000>\n", "",
     offsetof(SimulationConfig, clientRemoteCommandTimeoutMs), kRuntimeRemoteTimeoutMinMs,
     kRuntimeRemoteTimeoutMaxMs, true, true},
    {SimulationOptionGroup::Client, OptionKind::Uint, "--server-status-timeout-ms", "",
     "client_remote_status_timeout_ms", "", "", "  --server-status-timeout-ms <10..60000>\n", "",
     offsetof(SimulationConfig, clientRemoteStatusTimeoutMs), kRuntimeRemoteTimeoutMinMs,
     kRuntimeRemoteTimeoutMaxMs, true, true},
    {SimulationOptionGroup::Client, OptionKind::Uint, "--server-snapshot-timeout-ms", "",
     "client_remote_snapshot_timeout_ms", "", "", "  --server-snapshot-timeout-ms <10..60000>\n",
     "", offsetof(SimulationConfig, clientRemoteSnapshotTimeoutMs), kRuntimeRemoteTimeoutMinMs,
     kRuntimeRemoteTimeoutMaxMs, true, true},
    {SimulationOptionGroup::Client, OptionKind::Uint, "--snapshot-queue-capacity", "",
     "client_snapshot_queue_capacity", "", "", "  --snapshot-queue-capacity <n>\n", "",
     offsetof(SimulationConfig, clientSnapshotQueueCapacity), 1.0, 64.0, true, true},
    {SimulationOptionGroup::Client, OptionKind::String, "--snapshot-drop-policy", "",
     "client_snapshot_drop_policy", "", "", "  --snapshot-drop-policy <latest-only|paced>\n", "",
     offsetof(SimulationConfig, clientSnapshotDropPolicy), 0.0, 0.0, false, false},
    {SimulationOptionGroup::Client, OptionKind::String, "--export-directory", "",
     "export_directory", "", "", "  --export-directory <path>\n", "",
     offsetof(SimulationConfig, exportDirectory), 0.0, 0.0, false, false},
    {SimulationOptionGroup::Client, OptionKind::String, "--export-format", "", "export_format", "",
     "", "  --export-format <vtk|vtk_binary|xyz|bin>\n", "",
     offsetof(SimulationConfig, exportFormat), 0.0, 0.0, false, false},
    {SimulationOptionGroup::Client, OptionKind::String, "--input-file", "", "input_file", "", "",
     "  --input-file <path>\n", "", offsetof(SimulationConfig, inputFile), 0.0, 0.0, false, false},
    {SimulationOptionGroup::Client, OptionKind::String, "--input-format", "", "input_format", "",
     "", "  --input-format <auto|vtk|vtk_binary|xyz|bin>\n", "",
     offsetof(SimulationConfig, inputFormat), 0.0, 0.0, false, false},
    {SimulationOptionGroup::Client, OptionKind::String, "--init-config-style", "",
     "init_config_style", "", "", "  --init-config-style <preset|detailed>\n", "",
     offsetof(SimulationConfig, initConfigStyle), 0.0, 0.0, false, false},
    {SimulationOptionGroup::Client, OptionKind::String, "--preset-structure", "--structure",
     "preset_structure", "", "",
     "  --preset-structure "
     "<disk_orbit|galaxy|galaxy_collision|cosmology|random_cloud|cube_random|sphere_random|two_"
     "body|three_body|"
     "plummer_sphere|binary_star|solar_system|sph_collapse|file>"
     "\n",
     "  --structure "
     "<disk_orbit|galaxy|galaxy_collision|cosmology|random_cloud|cube_random|sphere_random|two_"
     "body|three_body|"
     "plummer_sphere|binary_star|solar_system|sph_collapse|file> "
     "(alias)\n",
     offsetof(SimulationConfig, presetStructure), 0.0, 0.0, false, false},
    {SimulationOptionGroup::Client, OptionKind::Float, "--preset-size", "--size", "preset_size", "",
     "", "  --preset-size <float>\n", "  --size <float> (alias)\n",
     offsetof(SimulationConfig, presetSize), 0.01, 0.0, true, false},
    {SimulationOptionGroup::Client, OptionKind::Float, "--velocity-temperature", "",
     "velocity_temperature", "temperature", "", "  --velocity-temperature <float>\n", "",
     offsetof(SimulationConfig, velocityTemperature), 0.0, 0.0, true, false},
    {SimulationOptionGroup::Client, OptionKind::Float, "--particle-temperature", "",
     "particle_temperature", "", "", "  --particle-temperature <float>\n", "",
     offsetof(SimulationConfig, particleTemperature), 0.0, 0.0, true, false},
    {SimulationOptionGroup::Client, OptionKind::Float, "--thermal-ambient", "",
     "thermal_ambient_temperature", "", "", "  --thermal-ambient <float>\n", "",
     offsetof(SimulationConfig, thermalAmbientTemperature), 0.0, 0.0, true, false},
    {SimulationOptionGroup::Client, OptionKind::Float, "--thermal-specific-heat", "",
     "thermal_specific_heat", "", "", "  --thermal-specific-heat <float>\n", "",
     offsetof(SimulationConfig, thermalSpecificHeat), 0.000001, 0.0, true, false},
    {SimulationOptionGroup::Client, OptionKind::Float, "--thermal-heating", "",
     "thermal_heating_coeff", "", "", "  --thermal-heating <float>\n", "",
     offsetof(SimulationConfig, thermalHeatingCoeff), 0.0, 0.0, true, false},
    {SimulationOptionGroup::Client, OptionKind::Float, "--thermal-radiation", "",
     "thermal_radiation_coeff", "", "", "  --thermal-radiation <float>\n", "",
     offsetof(SimulationConfig, thermalRadiationCoeff), 0.0, 0.0, true, false},
};

extern const std::size_t kClientPrimaryOptionCount =
    sizeof(kClientPrimaryOptions) / sizeof(kClientPrimaryOptions[0]);

extern const SimulationOptionEntry kClientTailOptions[] = {
    {SimulationOptionGroup::Client, OptionKind::Bool, "--render-culling", "",
     "render_culling_enabled", "", "", "  --render-culling <true|false>\n", "",
     offsetof(SimulationConfig, renderCullingEnabled), 0.0, 0.0, false, false},
    {SimulationOptionGroup::Client, OptionKind::Bool, "--render-lod", "", "render_lod_enabled", "",
     "", "  --render-lod <true|false>\n", "", offsetof(SimulationConfig, renderLODEnabled), 0.0,
     0.0, false, false},
    {SimulationOptionGroup::Client, OptionKind::Float, "--render-lod-near", "",
     "render_lod_near_distance", "", "", "  --render-lod-near <float>\n", "",
     offsetof(SimulationConfig, renderLODNearDistance), 0.0, 0.0, true, false},
    {SimulationOptionGroup::Client, OptionKind::Float, "--render-lod-far", "",
     "render_lod_far_distance", "", "", "  --render-lod-far <float>\n", "",
     offsetof(SimulationConfig, renderLODFarDistance), 0.0, 0.0, true, false},
};

extern const std::size_t kClientTailOptionCount =
    sizeof(kClientTailOptions) / sizeof(kClientTailOptions[0]);

} // namespace bltzr_config
