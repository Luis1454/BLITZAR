/*
 * @file tests/unit/config/args_io.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Automated verification assets for BLITZAR quality gates.
 */

#include "config/SimulationConfig.hpp"
#include "config/SimulationScenarioValidation.hpp"
#include "protocol/ServerProtocol.hpp"
#include "server/SimulationInitConfig.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <iostream>
#include <sstream>
#include <string>

TEST(ConfigArgsTest, TST_UNT_CONF_010_SimulationConfigSaveLoadRoundTrip)
{
    SimulationConfig config = SimulationConfig::defaults();
    config.particleCount = 321u;
    config.dt = 0.015f;
    config.solver = "octree_cpu";
    config.integrator = "rk4";
    config.substepTargetDt = 0.004f;
    config.maxSubsteps = 11u;
    config.sphEnabled = true;
    config.exportFormat = "bin";
    config.energySampleLimit = 777u;
    config.clientRemoteCommandTimeoutMs = 95u;
    config.clientRemoteStatusTimeoutMs = 50u;
    config.clientRemoteSnapshotTimeoutMs = 200u;
    config.clientSnapshotQueueCapacity = 6u;
    config.clientSnapshotDropPolicy = "paced";
    config.uiTheme = "dark";
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() /
        ("gravity_config_roundtrip_" + std::to_string(stamp) + ".ini");
    ASSERT_TRUE(config.save(path.string()));
    const SimulationConfig loaded = SimulationConfig::loadOrCreate(path.string());
    EXPECT_EQ(loaded.particleCount, config.particleCount);
    EXPECT_FLOAT_EQ(loaded.dt, config.dt);
    EXPECT_EQ(loaded.solver, config.solver);
    EXPECT_EQ(loaded.integrator, config.integrator);
    EXPECT_EQ(loaded.performanceProfile, "custom");
    EXPECT_FLOAT_EQ(loaded.substepTargetDt, config.substepTargetDt);
    EXPECT_EQ(loaded.maxSubsteps, config.maxSubsteps);
    EXPECT_EQ(loaded.snapshotPublishPeriodMs, config.snapshotPublishPeriodMs);
    EXPECT_EQ(loaded.sphEnabled, config.sphEnabled);
    EXPECT_EQ(loaded.exportFormat, config.exportFormat);
    EXPECT_EQ(loaded.energyMeasureEverySteps, config.energyMeasureEverySteps);
    EXPECT_EQ(loaded.energySampleLimit, config.energySampleLimit);
    EXPECT_EQ(loaded.clientRemoteCommandTimeoutMs, config.clientRemoteCommandTimeoutMs);
    EXPECT_EQ(loaded.clientRemoteStatusTimeoutMs, config.clientRemoteStatusTimeoutMs);
    EXPECT_EQ(loaded.clientRemoteSnapshotTimeoutMs, config.clientRemoteSnapshotTimeoutMs);
    EXPECT_EQ(loaded.clientSnapshotQueueCapacity, config.clientSnapshotQueueCapacity);
    EXPECT_EQ(loaded.clientSnapshotDropPolicy, config.clientSnapshotDropPolicy);
    EXPECT_EQ(loaded.uiTheme, config.uiTheme);
    std::error_code ec;
    std::filesystem::remove(path, ec);
}

TEST(ConfigArgsTest, TST_UNT_CONF_011_LoadIgnoresTrailingGarbageInNumericConfigValues)
{
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() /
        ("gravity_config_invalid_numbers_" + std::to_string(stamp) + ".ini");
    {
        std::ofstream out(path, std::ios::trunc);
        ASSERT_TRUE(out.is_open());
        out << "particle_count=123abc\n";
        out << "dt=0.25oops\n";
        out << "default_luminosity=220x\n";
        out << "solver=octree_cpu\n";
        out << "sph_enabled=true\n";
        out << "energy_sample_limit=2048\n";
    }
    const SimulationConfig defaults = SimulationConfig::defaults();
    const SimulationConfig loaded = SimulationConfig::loadOrCreate(path.string());
    EXPECT_EQ(loaded.particleCount, defaults.particleCount);
    EXPECT_FLOAT_EQ(loaded.dt, defaults.dt);
    EXPECT_EQ(loaded.defaultLuminosity, defaults.defaultLuminosity);
    EXPECT_EQ(loaded.solver, "octree_cpu");
    EXPECT_TRUE(loaded.sphEnabled);
    EXPECT_EQ(loaded.energySampleLimit, 2048u);
    std::error_code ec;
    std::filesystem::remove(path, ec);
}

TEST(ConfigArgsTest, TST_UNT_CONF_012_LoadOrCreateRejectsInvalidSolverAndIntegrator)
{
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() /
        ("gravity_config_invalid_modes_" + std::to_string(stamp) + ".ini");
    {
        std::ofstream out(path, std::ios::trunc);
        ASSERT_TRUE(out.is_open());
        out << "solver=bad_solver\n";
        out << "integrator=bad_integrator\n";
    }
    const SimulationConfig defaults = SimulationConfig::defaults();
    const SimulationConfig loaded = SimulationConfig::loadOrCreate(path.string());
    EXPECT_EQ(loaded.solver, defaults.solver);
    EXPECT_EQ(loaded.integrator, defaults.integrator);
    std::error_code ec;
    std::filesystem::remove(path, ec);
    const std::filesystem::path incompatiblePath =
        std::filesystem::temp_directory_path() /
        ("gravity_config_unsupported_modes_" + std::to_string(stamp) + ".ini");
    {
        std::ofstream out(incompatiblePath, std::ios::trunc);
        ASSERT_TRUE(out.is_open());
        out << "solver=octree_gpu\n";
        out << "integrator=rk4\n";
    }
    std::stringstream err;
    std::streambuf* previous = std::cerr.rdbuf(err.rdbuf());
    const SimulationConfig incompatibleLoaded =
        SimulationConfig::loadOrCreate(incompatiblePath.string());
    std::cerr.rdbuf(previous);
    EXPECT_EQ(incompatibleLoaded.solver, "octree_gpu");
    EXPECT_EQ(incompatibleLoaded.integrator, "euler");
    EXPECT_NE(err.str().find("unsupported solver/integrator combination"), std::string::npos);
    std::filesystem::remove(incompatiblePath, ec);
}

TEST(ConfigArgsTest, TST_UNT_CONF_013_LoadOrCreateCreatesFileWhenMissing)
{
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::filesystem::path path = std::filesystem::temp_directory_path() /
                                       ("gravity_config_create_" + std::to_string(stamp) + ".ini");
    ASSERT_FALSE(std::filesystem::exists(path));
    const SimulationConfig loaded = SimulationConfig::loadOrCreate(path.string());
    EXPECT_TRUE(std::filesystem::exists(path));
    EXPECT_EQ(loaded.solver, "pairwise_cuda");
    std::error_code ec;
    std::filesystem::remove(path, ec);
}

TEST(ConfigArgsTest, TST_UNT_CONF_015_DefaultClientParticleCapMatchesProtocolMax)
{
    const SimulationConfig defaults = SimulationConfig::defaults();
    EXPECT_EQ(defaults.performanceProfile, "interactive");
    EXPECT_EQ(defaults.clientParticleCap, grav_protocol::kSnapshotDefaultPoints);
    EXPECT_EQ(defaults.snapshotPublishPeriodMs, 50u);
    EXPECT_FLOAT_EQ(defaults.substepTargetDt, 0.01f);
    EXPECT_EQ(defaults.maxSubsteps, 4u);
}

TEST(ConfigArgsTest, TST_UNT_CONF_016_LoadClampsClientParticleCapToProtocolMax)
{
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() /
        ("gravity_config_client_cap_" + std::to_string(stamp) + ".ini");
    {
        std::ofstream out(path, std::ios::trunc);
        ASSERT_TRUE(out.is_open());
        out << "client_particle_cap=50000\n";
    }
    std::stringstream err;
    std::streambuf* previous = std::cerr.rdbuf(err.rdbuf());
    const SimulationConfig loaded = SimulationConfig::loadOrCreate(path.string());
    std::cerr.rdbuf(previous);
    EXPECT_EQ(loaded.clientParticleCap, grav_protocol::kSnapshotMaxPoints);
    EXPECT_NE(err.str().find("client_particle_cap clamped"), std::string::npos);
    std::error_code ec;
    std::filesystem::remove(path, ec);
}

TEST(ConfigArgsTest, TST_UNT_CONF_017_LoadWarnsOnUnknownIniKey)
{
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() /
        ("gravity_config_unknown_key_" + std::to_string(stamp) + ".ini");
    {
        std::ofstream out(path, std::ios::trunc);
        ASSERT_TRUE(out.is_open());
        out << "mystery_option=42\n";
    }
    std::stringstream err;
    std::streambuf* previous = std::cerr.rdbuf(err.rdbuf());
    const SimulationConfig loaded = SimulationConfig::loadOrCreate(path.string());
    std::cerr.rdbuf(previous);
    EXPECT_EQ(loaded.particleCount, SimulationConfig::defaults().particleCount);
    EXPECT_NE(err.str().find("unknown key ignored: mystery_option"), std::string::npos);
    std::error_code ec;
    std::filesystem::remove(path, ec);
}

TEST(ConfigArgsTest, TST_UNT_CONF_018_LoadSupportsRegistryAliases)
{
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::filesystem::path path = std::filesystem::temp_directory_path() /
                                       ("gravity_config_aliases_" + std::to_string(stamp) + ".ini");
    {
        std::ofstream out(path, std::ios::trunc);
        ASSERT_TRUE(out.is_open());
        out << "temperature=0.8\n";
        out << "client_remote_timeout_ms=120\n";
    }
    const SimulationConfig loaded = SimulationConfig::loadOrCreate(path.string());
    EXPECT_FLOAT_EQ(loaded.velocityTemperature, 0.8f);
    EXPECT_EQ(loaded.clientRemoteCommandTimeoutMs, 120u);
    EXPECT_EQ(loaded.clientRemoteStatusTimeoutMs, 120u);
    EXPECT_EQ(loaded.clientRemoteSnapshotTimeoutMs, 120u);
    std::error_code ec;
    std::filesystem::remove(path, ec);
}

TEST(ConfigArgsTest, TST_UNT_CONF_020_ResolveInitPlanRejectsFileModeWithoutInput)
{
    SimulationConfig config = SimulationConfig::defaults();
    config.initConfigStyle = "preset";
    config.initMode = "file";
    std::stringstream log;
    const ResolvedInitialStatePlan plan = resolveInitialStatePlan(config, log);
    EXPECT_EQ(plan.config.mode, "disk_orbit");
    EXPECT_TRUE(plan.inputFile.empty());
    EXPECT_NE(log.str().find("init_mode=file ignored"), std::string::npos);
    EXPECT_NE(plan.summary.find("mode=disk_orbit"), std::string::npos);
    SimulationConfig invalidParticleConfig = SimulationConfig::defaults();
    invalidParticleConfig.particleCount = 1u;
    const grav_config::ScenarioValidationReport report =
        grav_config::SimulationScenarioValidation::evaluate(invalidParticleConfig);
    EXPECT_FALSE(report.validForRun);
    EXPECT_EQ(report.errorCount, 1u);
    EXPECT_NE(grav_config::SimulationScenarioValidation::renderText(report).find("particle_count"),
              std::string::npos);
}

TEST(ConfigArgsTest, TST_UNT_CONF_021_ResolveInitPlanIgnoresStaleInputFileOutsideFileMode)
{
    SimulationConfig config = SimulationConfig::defaults();
    config.initConfigStyle = "detailed";
    config.initMode = "random_cloud";
    config.presetStructure = "file";
    config.inputFile = "tests/data/two_body_rest.xyz";
    std::stringstream log;
    const ResolvedInitialStatePlan plan = resolveInitialStatePlan(config, log);
    const grav_config::ScenarioValidationReport report =
        grav_config::SimulationScenarioValidation::evaluate(config);
    EXPECT_EQ(plan.config.mode, "random_cloud");
    EXPECT_TRUE(plan.inputFile.empty());
    EXPECT_NE(log.str().find("preset_structure=file ignored"), std::string::npos);
    EXPECT_NE(log.str().find("input_file ignored"), std::string::npos);
    EXPECT_NE(plan.summary.find("source=generated"), std::string::npos);
    EXPECT_TRUE(report.validForRun);
    EXPECT_GE(report.warningCount, 1u);
    EXPECT_NE(grav_config::SimulationScenarioValidation::renderText(report).find("initial_state"),
              std::string::npos);
}

TEST(ConfigArgsTest, TST_UNT_CONF_023_ResolveInitPlanSupportsCalibrationPresets)
{
    SimulationConfig config = SimulationConfig::defaults();
    config.initConfigStyle = "preset";
    config.presetStructure = "three_body";
    config.presetSize = 3.5f;
    const ResolvedInitialStatePlan presetPlan = resolveInitialStatePlan(config, std::cerr);
    EXPECT_EQ(presetPlan.config.mode, "three_body");
    EXPECT_FALSE(presetPlan.config.includeCentralBody);
    EXPECT_FLOAT_EQ(presetPlan.config.cloudHalfExtent, 3.5f);
    config.initConfigStyle = "detailed";
    config.initMode = "plummer_sphere";
    config.initCloudHalfExtent = 9.0f;
    config.initParticleMass = 0.02f;
    const ResolvedInitialStatePlan detailedPlan = resolveInitialStatePlan(config, std::cerr);
    EXPECT_EQ(detailedPlan.config.mode, "plummer_sphere");
    EXPECT_FLOAT_EQ(detailedPlan.config.cloudHalfExtent, 9.0f);
    EXPECT_FLOAT_EQ(detailedPlan.config.particleMass, 0.02f);
}
