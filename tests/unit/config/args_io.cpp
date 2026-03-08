#include "config/SimulationConfig.hpp"

#include "protocol/BackendProtocol.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
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
    config.sphEnabled = true;
    config.exportFormat = "bin";
    config.energySampleLimit = 777u;
    config.frontendRemoteCommandTimeoutMs = 95u;
    config.frontendRemoteStatusTimeoutMs = 50u;
    config.frontendRemoteSnapshotTimeoutMs = 200u;

    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / ("gravity_config_roundtrip_" + std::to_string(stamp) + ".ini");

    ASSERT_TRUE(config.save(path.string()));

    const SimulationConfig loaded = SimulationConfig::loadOrCreate(path.string());
    EXPECT_EQ(loaded.particleCount, config.particleCount);
    EXPECT_FLOAT_EQ(loaded.dt, config.dt);
    EXPECT_EQ(loaded.solver, config.solver);
    EXPECT_EQ(loaded.integrator, config.integrator);
    EXPECT_EQ(loaded.sphEnabled, config.sphEnabled);
    EXPECT_EQ(loaded.exportFormat, config.exportFormat);
    EXPECT_EQ(loaded.energySampleLimit, config.energySampleLimit);
    EXPECT_EQ(loaded.frontendRemoteCommandTimeoutMs, config.frontendRemoteCommandTimeoutMs);
    EXPECT_EQ(loaded.frontendRemoteStatusTimeoutMs, config.frontendRemoteStatusTimeoutMs);
    EXPECT_EQ(loaded.frontendRemoteSnapshotTimeoutMs, config.frontendRemoteSnapshotTimeoutMs);

    std::error_code ec;
    std::filesystem::remove(path, ec);
}

TEST(ConfigArgsTest, TST_UNT_CONF_011_LoadIgnoresTrailingGarbageInNumericConfigValues)
{
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::filesystem::path path = std::filesystem::temp_directory_path()
        / ("gravity_config_invalid_numbers_" + std::to_string(stamp) + ".ini");

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
        std::filesystem::temp_directory_path() / ("gravity_config_invalid_modes_" + std::to_string(stamp) + ".ini");

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
}

TEST(ConfigArgsTest, TST_UNT_CONF_013_LoadOrCreateCreatesFileWhenMissing)
{
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / ("gravity_config_create_" + std::to_string(stamp) + ".ini");

    ASSERT_FALSE(std::filesystem::exists(path));
    const SimulationConfig loaded = SimulationConfig::loadOrCreate(path.string());
    EXPECT_TRUE(std::filesystem::exists(path));
    EXPECT_EQ(loaded.solver, "pairwise_cuda");

    std::error_code ec;
    std::filesystem::remove(path, ec);
}

TEST(ConfigArgsTest, TST_UNT_CONF_015_DefaultFrontendParticleCapMatchesProtocolMax)
{
    const SimulationConfig defaults = SimulationConfig::defaults();
    EXPECT_EQ(defaults.frontendParticleCap, grav_protocol::kSnapshotMaxPoints);
}

TEST(ConfigArgsTest, TST_UNT_CONF_016_LoadClampsFrontendParticleCapToProtocolMax)
{
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / ("gravity_config_frontend_cap_" + std::to_string(stamp) + ".ini");

    {
        std::ofstream out(path, std::ios::trunc);
        ASSERT_TRUE(out.is_open());
        out << "frontend_particle_cap=50000\n";
    }

    std::stringstream err;
    std::streambuf *previous = std::cerr.rdbuf(err.rdbuf());
    const SimulationConfig loaded = SimulationConfig::loadOrCreate(path.string());
    std::cerr.rdbuf(previous);

    EXPECT_EQ(loaded.frontendParticleCap, grav_protocol::kSnapshotMaxPoints);
    EXPECT_NE(err.str().find("frontend_particle_cap clamped"), std::string::npos);

    std::error_code ec;
    std::filesystem::remove(path, ec);
}

