// File: tests/unit/config/args_io_validation.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "config/SimulationConfig.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <sstream>
#include <string>
/// Description: Executes the TEST operation.
TEST(ConfigArgsTest, TST_UNT_CONF_034_LoadOrCreateReportsSiValidationDiagnostics)
{
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() /
        ("gravity_config_si_validation_" + std::to_string(stamp) + ".ini");
    {
        /// Description: Executes the out operation.
        std::ofstream out(path, std::ios::trunc);
        /// Description: Executes the ASSERT_TRUE operation.
        ASSERT_TRUE(out.is_open());
        out << "particle_count=128\n";
        out << "dt=0.5\n";
        out << "solver=octree_cpu\n";
        out << "integrator=euler\n";
        out << "octree_theta=1.2\n";
        out << "octree_softening=0.25\n";
        out << "physics_min_softening=0.3\n";
        out << "physics_min_distance2=0.2\n";
        out << "preset_size=10\n";
        out << "velocity_temperature=40\n";
        out << "particle_temperature=0\n";
    }
    std::stringstream err;
    std::streambuf* previous = std::cerr.rdbuf(err.rdbuf());
    const SimulationConfig loaded = SimulationConfig::loadOrCreate(path.string());
    std::cerr.rdbuf(previous);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(loaded.particleCount, 128u);
    const std::string log = err.str();
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(log.find("[preflight] warnings"), std::string::npos);
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(log.find("dt [s] * velocity [m/s]"), std::string::npos);
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(log.find("octree_softening [m]"), std::string::npos);
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(log.find("physics_min_distance2 [m^2]"), std::string::npos);
    std::error_code ec;
    /// Description: Executes the remove operation.
    std::filesystem::remove(path, ec);
}
/// Description: Executes the TEST operation.
TEST(ConfigArgsTest, TST_UNT_CONF_035_LoadValidatesSnapshotPipelineClientSettings)
{
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() /
        ("gravity_config_snapshot_pipeline_" + std::to_string(stamp) + ".ini");
    {
        /// Description: Executes the out operation.
        std::ofstream out(path, std::ios::trunc);
        /// Description: Executes the ASSERT_TRUE operation.
        ASSERT_TRUE(out.is_open());
        out << "client_snapshot_queue_capacity=0\n";
        out << "client_snapshot_drop_policy=oldest\n";
    }
    std::stringstream err;
    std::streambuf* previous = std::cerr.rdbuf(err.rdbuf());
    const SimulationConfig loaded = SimulationConfig::loadOrCreate(path.string());
    std::cerr.rdbuf(previous);
    EXPECT_EQ(loaded.clientSnapshotQueueCapacity,
              /// Description: Executes the defaults operation.
              SimulationConfig::defaults().clientSnapshotQueueCapacity);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(loaded.clientSnapshotDropPolicy, "oldest");
    const std::string log = err.str();
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(log.find("client_snapshot_queue_capacity"), std::string::npos);
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(log.find("client_snapshot_drop_policy"), std::string::npos);
    std::error_code ec;
    /// Description: Executes the remove operation.
    std::filesystem::remove(path, ec);
}
