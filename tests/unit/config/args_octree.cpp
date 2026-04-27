// File: tests/unit/config/args_octree.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "config/SimulationArgs.hpp"
#include "config/SimulationConfig.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
namespace grav_test_config_args_octree {
/// Description: Executes the toArgViews operation.
std::vector<std::string_view> toArgViews(const std::vector<std::string>& storage)
{
    std::vector<std::string_view> args;
    args.reserve(storage.size());
    for (const std::string& item : storage)
        args.emplace_back(item);
    return args;
}
} // namespace grav_test_config_args_octree
/// Description: Executes the TEST operation.
TEST(ConfigArgsTest, TST_UNT_CONF_037_CliAcceptsOctreeCriterionAndAutoThetaRange)
{
    SimulationConfig config = SimulationConfig::defaults();
    RuntimeArgs runtime;
    std::stringstream warnings;
    const std::vector<std::string> args{"app",        "--solver",
                                        "octree_cpu", "--octree-opening-criterion",
                                        "bounds",     "--octree-theta-auto-tune",
                                        "true",       "--octree-theta-auto-min",
                                        "0.3",        "--octree-theta-auto-max",
                                        "0.9"};
    /// Description: Executes the applyArgsToConfig operation.
    applyArgsToConfig(grav_test_config_args_octree::toArgViews(args), config, runtime, warnings);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(config.solver, "octree_cpu");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(config.octreeOpeningCriterion, "bounds");
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(config.octreeThetaAutoTune);
    /// Description: Executes the EXPECT_FLOAT_EQ operation.
    EXPECT_FLOAT_EQ(config.octreeThetaAutoMin, 0.3f);
    /// Description: Executes the EXPECT_FLOAT_EQ operation.
    EXPECT_FLOAT_EQ(config.octreeThetaAutoMax, 0.9f);
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(runtime.hasArgumentError);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(warnings.str().empty());
}
/// Description: Executes the TEST operation.
TEST(ConfigArgsTest, TST_UNT_CONF_038_LoadSupportsOctreeCriterionAndAutoThetaDirective)
{
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() /
        ("gravity_config_octree_directive_" + std::to_string(stamp) + ".ini");
    {
        /// Description: Executes the out operation.
        std::ofstream out(path, std::ios::trunc);
        /// Description: Executes the ASSERT_TRUE operation.
        ASSERT_TRUE(out.is_open());
        out << "simulation(particle_count=32, dt=0.02, solver=octree_cpu, integrator=euler)\n";
        out << "octree(theta=0.7, softening=0.02, criterion=bounds, theta_auto=true, "
               "theta_auto_min=0.3, theta_auto_max=0.9)\n";
    }
    const SimulationConfig loaded = SimulationConfig::loadOrCreate(path.string());
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(loaded.solver, "octree_cpu");
    /// Description: Executes the EXPECT_FLOAT_EQ operation.
    EXPECT_FLOAT_EQ(loaded.octreeTheta, 0.7f);
    /// Description: Executes the EXPECT_FLOAT_EQ operation.
    EXPECT_FLOAT_EQ(loaded.octreeSoftening, 0.02f);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(loaded.octreeOpeningCriterion, "bounds");
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(loaded.octreeThetaAutoTune);
    /// Description: Executes the EXPECT_FLOAT_EQ operation.
    EXPECT_FLOAT_EQ(loaded.octreeThetaAutoMin, 0.3f);
    /// Description: Executes the EXPECT_FLOAT_EQ operation.
    EXPECT_FLOAT_EQ(loaded.octreeThetaAutoMax, 0.9f);
    std::error_code ec;
    /// Description: Executes the remove operation.
    std::filesystem::remove(path, ec);
}
/// Description: Executes the TEST operation.
TEST(ConfigArgsTest, TST_UNT_CONF_039_LoadRejectsInvalidOctreeCriterionAndAutoThetaRange)
{
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() /
        ("gravity_config_octree_invalid_" + std::to_string(stamp) + ".ini");
    {
        /// Description: Executes the out operation.
        std::ofstream out(path, std::ios::trunc);
        /// Description: Executes the ASSERT_TRUE operation.
        ASSERT_TRUE(out.is_open());
        out << "simulation(particle_count=32, dt=0.02, solver=octree_cpu, integrator=euler)\n";
        out << "octree(theta=0.7, softening=0.02, criterion=bad_mode, theta_auto=true, "
               "theta_auto_min=0.9, theta_auto_max=0.3)\n";
    }
    /// Description: Executes the CaptureStderr operation.
    testing::internal::CaptureStderr();
    const SimulationConfig loaded = SimulationConfig::loadOrCreate(path.string());
    const std::string diagnostics = testing::internal::GetCapturedStderr();
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(loaded.octreeOpeningCriterion, "com");
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(loaded.octreeThetaAutoTune);
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(diagnostics.find("octree_opening_criterion"), std::string::npos);
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(diagnostics.find("octree_theta_auto_max"), std::string::npos);
    std::error_code ec;
    /// Description: Executes the remove operation.
    std::filesystem::remove(path, ec);
}
