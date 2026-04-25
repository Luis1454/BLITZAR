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
std::vector<std::string_view> toArgViews(const std::vector<std::string>& storage)
{
    std::vector<std::string_view> args;
    args.reserve(storage.size());
    for (const std::string& item : storage)
        args.emplace_back(item);
    return args;
}
} // namespace grav_test_config_args_octree
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
    applyArgsToConfig(grav_test_config_args_octree::toArgViews(args), config, runtime, warnings);
    EXPECT_EQ(config.solver, "octree_cpu");
    EXPECT_EQ(config.octreeOpeningCriterion, "bounds");
    EXPECT_TRUE(config.octreeThetaAutoTune);
    EXPECT_FLOAT_EQ(config.octreeThetaAutoMin, 0.3f);
    EXPECT_FLOAT_EQ(config.octreeThetaAutoMax, 0.9f);
    EXPECT_FALSE(runtime.hasArgumentError);
    EXPECT_TRUE(warnings.str().empty());
}
TEST(ConfigArgsTest, TST_UNT_CONF_038_LoadSupportsOctreeCriterionAndAutoThetaDirective)
{
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() /
        ("gravity_config_octree_directive_" + std::to_string(stamp) + ".ini");
    {
        std::ofstream out(path, std::ios::trunc);
        ASSERT_TRUE(out.is_open());
        out << "simulation(particle_count=32, dt=0.02, solver=octree_cpu, integrator=euler)\n";
        out << "octree(theta=0.7, softening=0.02, criterion=bounds, theta_auto=true, "
               "theta_auto_min=0.3, theta_auto_max=0.9)\n";
    }
    const SimulationConfig loaded = SimulationConfig::loadOrCreate(path.string());
    EXPECT_EQ(loaded.solver, "octree_cpu");
    EXPECT_FLOAT_EQ(loaded.octreeTheta, 0.7f);
    EXPECT_FLOAT_EQ(loaded.octreeSoftening, 0.02f);
    EXPECT_EQ(loaded.octreeOpeningCriterion, "bounds");
    EXPECT_TRUE(loaded.octreeThetaAutoTune);
    EXPECT_FLOAT_EQ(loaded.octreeThetaAutoMin, 0.3f);
    EXPECT_FLOAT_EQ(loaded.octreeThetaAutoMax, 0.9f);
    std::error_code ec;
    std::filesystem::remove(path, ec);
}
TEST(ConfigArgsTest, TST_UNT_CONF_039_LoadRejectsInvalidOctreeCriterionAndAutoThetaRange)
{
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() /
        ("gravity_config_octree_invalid_" + std::to_string(stamp) + ".ini");
    {
        std::ofstream out(path, std::ios::trunc);
        ASSERT_TRUE(out.is_open());
        out << "simulation(particle_count=32, dt=0.02, solver=octree_cpu, integrator=euler)\n";
        out << "octree(theta=0.7, softening=0.02, criterion=bad_mode, theta_auto=true, "
               "theta_auto_min=0.9, theta_auto_max=0.3)\n";
    }
    testing::internal::CaptureStderr();
    const SimulationConfig loaded = SimulationConfig::loadOrCreate(path.string());
    const std::string diagnostics = testing::internal::GetCapturedStderr();
    EXPECT_EQ(loaded.octreeOpeningCriterion, "com");
    EXPECT_TRUE(loaded.octreeThetaAutoTune);
    EXPECT_NE(diagnostics.find("octree_opening_criterion"), std::string::npos);
    EXPECT_NE(diagnostics.find("octree_theta_auto_max"), std::string::npos);
    std::error_code ec;
    std::filesystem::remove(path, ec);
}
