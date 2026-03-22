#include "config/SimulationConfig.hpp"
#include "protocol/ServerProtocol.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

TEST(ConfigArgsTest, TST_UNT_CONF_024_LoadSupportsDirectiveSyntax)
{
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / ("gravity_config_directive_load_" + std::to_string(stamp) + ".ini");
    {
        std::ofstream out(path, std::ios::trunc);
        ASSERT_TRUE(out.is_open());
        out << "simulation(particles=123, dt=0.05, solver=octree_cpu, integrator=rk4)\n";
        out << "performance(profile=interactive)\n";
        out << "client(zoom=6, luminosity=120, ui_fps=55, command_timeout_ms=110, status_timeout_ms=45, snapshot_timeout_ms=190, snapshot_queue=5, drop_policy=paced)\n";
        out << "scene(style=preset, preset=file, mode=file, file=\"tests/data/two_body_rest.xyz\", format=auto)\n";
        out << "preset(size=8, velocity_temperature=0.2, temperature=1.5)\n";
        out << "thermal(ambient=2, specific_heat=3, heating=4, radiation=5)\n";
    }
    const SimulationConfig loaded = SimulationConfig::loadOrCreate(path.string());
    EXPECT_EQ(loaded.particleCount, 123u);
    EXPECT_FLOAT_EQ(loaded.dt, 0.05f);
    EXPECT_EQ(loaded.solver, "octree_cpu");
    EXPECT_EQ(loaded.integrator, "rk4");
    EXPECT_EQ(loaded.performanceProfile, "interactive");
    EXPECT_FLOAT_EQ(loaded.substepTargetDt, 0.01f);
    EXPECT_EQ(loaded.maxSubsteps, 4u);
    EXPECT_EQ(loaded.snapshotPublishPeriodMs, 50u);
    EXPECT_EQ(loaded.clientParticleCap, grav_protocol::kSnapshotDefaultPoints);
    EXPECT_EQ(loaded.uiFpsLimit, 55u);
    EXPECT_EQ(loaded.clientSnapshotQueueCapacity, 5u);
    EXPECT_EQ(loaded.clientSnapshotDropPolicy, "paced");
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
        std::filesystem::temp_directory_path() / ("gravity_config_directive_perf_override_" + std::to_string(stamp) + ".ini");
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
        std::filesystem::temp_directory_path() / ("gravity_config_directive_save_" + std::to_string(stamp) + ".ini");
    ASSERT_TRUE(config.save(path.string()));
    std::ifstream in(path);
    ASSERT_TRUE(in.is_open());
    const std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    EXPECT_NE(content.find("simulation("), std::string::npos);
    EXPECT_NE(content.find("performance("), std::string::npos);
    EXPECT_NE(content.find("scene("), std::string::npos);
    EXPECT_NE(content.find("thermal("), std::string::npos);
    EXPECT_NE(content.find("client(zoom=8, luminosity=100, ui_fps=60, command_timeout_ms=80, status_timeout_ms=40, snapshot_timeout_ms=140, snapshot_queue=6, drop_policy=paced)\n"), std::string::npos);
    EXPECT_NE(content.find("export(directory=\"exports final\", format=vtk)\n"), std::string::npos);
    EXPECT_NE(
        content.find("scene(style=preset, preset=three_body, mode=disk_orbit, file=\"exports/state final.vtk\", format=auto)\n"),
        std::string::npos
    );
    EXPECT_EQ(content.find("\nparticle_count="), std::string::npos);
    EXPECT_EQ(content.find("\nthermal_ambient_temperature="), std::string::npos);
    std::error_code ec;
    std::filesystem::remove(path, ec);
}
