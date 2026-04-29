/*
 * @file tests/unit/config/args_io_directive.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Automated verification assets for BLITZAR quality gates.
 */

#include "config/SimulationConfig.hpp"
#include "config/SimulationPerformanceProfile.hpp"
#include "protocol/ServerProtocol.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <string>

TEST(ConfigArgsTest, TST_UNT_CONF_024_LoadSupportsDirectiveSyntax)
{
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() /
        ("BLITZAR_config_directive_load_" + std::to_string(stamp) + ".ini");
    {
        std::ofstream out(path, std::ios::trunc);
        ASSERT_TRUE(out.is_open());
        out << "simulation(particles=123, dt=0.05, solver=octree_cpu, integrator=rk4)\n";
        out << "performance(profile=interactive)\n";
        out << "physics(max_acceleration=32, min_softening=0.0002, min_distance2=0.000000000001, "
               "min_theta=0.04)\n";
        out << "sph(enabled=true, smoothing_length=1.5, rest_density=2, gas_constant=5, "
               "viscosity=0.12, max_acceleration=41, max_speed=130)\n";
        out << "client(zoom=6, luminosity=120, theme=dark, ui_fps=55, command_timeout_ms=110, "
               "status_timeout_ms=45, snapshot_timeout_ms=190, snapshot_queue=5, "
               "drop_policy=paced)\n";
        out << "scene(style=preset, preset=file, mode=file, file=\"tests/data/two_body_rest.xyz\", "
               "format=auto)\n";
        out << "preset(size=8, velocity_temperature=0.2, temperature=1.5)\n";
        out << "thermal(ambient=2, specific_heat=3, heating=4, radiation=5)\n";
    }
    const SimulationConfig loaded = SimulationConfig::loadOrCreate(path.string());
    EXPECT_EQ(loaded.particleCount, 123u);
    EXPECT_FLOAT_EQ(loaded.dt, 0.05f);
    EXPECT_EQ(loaded.solver, "octree_cpu");
    EXPECT_EQ(loaded.integrator, "rk4");
    EXPECT_EQ(loaded.performanceProfile, "interactive");
    EXPECT_FLOAT_EQ(loaded.physicsMaxAcceleration, 32.0f);
    EXPECT_FLOAT_EQ(loaded.physicsMinSoftening, 0.0002f);
    EXPECT_FLOAT_EQ(loaded.physicsMinDistance2, 1.0e-12f);
    EXPECT_FLOAT_EQ(loaded.physicsMinTheta, 0.04f);
    EXPECT_TRUE(loaded.sphEnabled);
    EXPECT_FLOAT_EQ(loaded.sphSmoothingLength, 1.5f);
    EXPECT_FLOAT_EQ(loaded.sphRestDensity, 2.0f);
    EXPECT_FLOAT_EQ(loaded.sphGasConstant, 5.0f);
    EXPECT_FLOAT_EQ(loaded.sphViscosity, 0.12f);
    EXPECT_FLOAT_EQ(loaded.sphMaxAcceleration, 41.0f);
    EXPECT_FLOAT_EQ(loaded.sphMaxSpeed, 130.0f);
    EXPECT_FLOAT_EQ(loaded.substepTargetDt, 0.01f);
    EXPECT_EQ(loaded.maxSubsteps, 4u);
    EXPECT_EQ(loaded.snapshotPublishPeriodMs, 50u);
    EXPECT_EQ(loaded.clientParticleCap, bltzr_protocol::kSnapshotDefaultPoints);
    EXPECT_EQ(loaded.uiFpsLimit, 55u);
    EXPECT_EQ(loaded.clientSnapshotQueueCapacity, 5u);
    EXPECT_EQ(loaded.clientSnapshotDropPolicy, "paced");
    EXPECT_EQ(loaded.uiTheme, "dark");
    EXPECT_EQ(loaded.inputFile, "tests/data/two_body_rest.xyz");
    EXPECT_EQ(loaded.presetStructure, "file");
    EXPECT_FLOAT_EQ(loaded.presetSize, 8.0f);
    EXPECT_FLOAT_EQ(loaded.thermalAmbientTemperature, 2.0f);
    EXPECT_FLOAT_EQ(loaded.thermalRadiationCoeff, 5.0f);
    std::error_code ec;
    std::filesystem::remove(path, ec);
}

TEST(ConfigArgsTest, TST_UNT_CONF_027_DirectivePerformanceOverrideForcesCustomProfile)
{
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() /
        ("BLITZAR_config_directive_perf_override_" + std::to_string(stamp) + ".ini");
    {
        std::ofstream out(path, std::ios::trunc);
        ASSERT_TRUE(out.is_open());
        out << "performance(profile=interactive)\n";
        out << "substeps(target_dt=0.004, max=6)\n";
    }
    const SimulationConfig loaded = SimulationConfig::loadOrCreate(path.string());
    EXPECT_EQ(loaded.performanceProfile, "custom");
    EXPECT_FLOAT_EQ(loaded.substepTargetDt, 0.004f);
    EXPECT_EQ(loaded.maxSubsteps, 6u);
    EXPECT_EQ(loaded.snapshotPublishPeriodMs, 50u);
    EXPECT_EQ(loaded.energyMeasureEverySteps, 30u);
    EXPECT_EQ(loaded.energySampleLimit, 256u);
    std::error_code ec;
    std::filesystem::remove(path, ec);
}

TEST(ConfigArgsTest, TST_UNT_CONF_025_SaveWritesDirectiveSyntax)
{
    SimulationConfig config = SimulationConfig::defaults();
    config.particleCount = 64u;
    config.solver = "octree_gpu";
    config.integrator = "euler";
    config.exportDirectory = "exports final";
    config.presetStructure = "three_body";
    config.inputFile = "exports/state final.vtk";
    config.thermalRadiationCoeff = 0.4f;
    config.clientSnapshotQueueCapacity = 6u;
    config.clientSnapshotDropPolicy = "paced";
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() /
        ("BLITZAR_config_directive_save_" + std::to_string(stamp) + ".ini");
    ASSERT_TRUE(config.save(path.string()));
    std::ifstream in(path);
    ASSERT_TRUE(in.is_open());
    const std::string content((std::istreambuf_iterator<char>(in)),
                              std::istreambuf_iterator<char>());
    EXPECT_NE(content.find("simulation("), std::string::npos);
    EXPECT_NE(content.find("performance("), std::string::npos);
    EXPECT_NE(content.find("scene("), std::string::npos);
    EXPECT_NE(content.find("thermal("), std::string::npos);
    EXPECT_NE(content.find("client(zoom=8, luminosity=100, theme=light, ui_fps=60, "
                           "command_timeout_ms=80, status_timeout_ms=40, snapshot_timeout_ms=140, "
                           "snapshot_queue=6, drop_policy=paced)\n"),
              std::string::npos);
    EXPECT_NE(content.find("export(directory=\"exports final\", format=vtk)\n"), std::string::npos);
    EXPECT_NE(content.find("scene(style=preset, preset=three_body, mode=disk_orbit, "
                           "file=\"exports/state final.vtk\", format=auto)\n"),
              std::string::npos);
    EXPECT_EQ(content.find("\nparticle_count="), std::string::npos);
    EXPECT_EQ(content.find("\nthermal_ambient_temperature="), std::string::npos);
    std::error_code ec;
    std::filesystem::remove(path, ec);
}

TEST(ConfigArgsTest, TST_UNT_CONF_068_LoadDirectiveFallbackHandlesMalformedDirectiveLine)
{
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() /
        ("BLITZAR_config_directive_invalid_" + std::to_string(stamp) + ".ini");
    {
        std::ofstream out(path, std::ios::trunc);
        ASSERT_TRUE(out.is_open());
        out << "simulation(particles)\n";
        out << "particle_count=77\n";
    }
    std::stringstream err;
    std::streambuf* previous = std::cerr.rdbuf(err.rdbuf());
    const SimulationConfig loaded = SimulationConfig::loadOrCreate(path.string());
    std::cerr.rdbuf(previous);
    EXPECT_EQ(loaded.particleCount, 77u);
    EXPECT_NE(err.str().find("[config] invalid line ignored: simulation(particles)"),
              std::string::npos);
    std::error_code ec;
    std::filesystem::remove(path, ec);
}

TEST(ConfigArgsTest, TST_UNT_CONF_069_LoadDirectiveWarnsUnknownDirectiveArgument)
{
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() /
        ("BLITZAR_config_directive_unknown_arg_" + std::to_string(stamp) + ".ini");
    {
        std::ofstream out(path, std::ios::trunc);
        ASSERT_TRUE(out.is_open());
        out << "client(theme=dark, mystery=42)\n";
    }
    std::stringstream err;
    std::streambuf* previous = std::cerr.rdbuf(err.rdbuf());
    const SimulationConfig loaded = SimulationConfig::loadOrCreate(path.string());
    std::cerr.rdbuf(previous);
    EXPECT_EQ(loaded.uiTheme, "dark");
    EXPECT_NE(err.str().find("[config] unknown directive argument ignored: client.mystery"),
              std::string::npos);
    std::error_code ec;
    std::filesystem::remove(path, ec);
}

TEST(ConfigArgsTest, TST_UNT_CONF_070_SaveDirectiveKeepsBalancedProfileWithoutCustomOverrides)
{
    SimulationConfig config = SimulationConfig::defaults();
    config.performanceProfile = "balanced";
    bltzr_config::applyPerformanceProfile(config);
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() /
        ("BLITZAR_config_directive_perf_balanced_" + std::to_string(stamp) + ".ini");
    ASSERT_TRUE(config.save(path.string()));
    std::ifstream in(path);
    ASSERT_TRUE(in.is_open());
    const std::string content((std::istreambuf_iterator<char>(in)),
                              std::istreambuf_iterator<char>());
    EXPECT_NE(content.find("performance(profile=balanced)\n"), std::string::npos);
    EXPECT_EQ(content.find("performance(profile=balanced, draw_cap="), std::string::npos);
    std::error_code ec;
    std::filesystem::remove(path, ec);
}
