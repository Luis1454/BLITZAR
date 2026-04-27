// File: tests/unit/config/args_cli_usage.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "config/SimulationArgs.hpp"
#include "config/SimulationConfig.hpp"
#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
namespace grav_test_config_args_cli_usage {
/// Description: Executes the toArgViews operation.
std::vector<std::string_view> toArgViews(const std::vector<std::string>& storage)
{
    std::vector<std::string_view> args;
    args.reserve(storage.size());
    for (const std::string& item : storage)
        args.emplace_back(item);
    return args;
}
} // namespace grav_test_config_args_cli_usage
/// Description: Executes the TEST operation.
TEST(ConfigArgsTest, TST_UNT_CONF_007_UsageIncludesCoreOptions)
{
    std::stringstream out;
    /// Description: Executes the printUsage operation.
    printUsage(out, "app", true);
    const std::string usage = out.str();
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(usage.find("--particle-count"), std::string::npos);
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(usage.find("--dt"), std::string::npos);
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(usage.find("--solver"), std::string::npos);
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(usage.find("--integrator"), std::string::npos);
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(usage.find("--server-command-timeout-ms"), std::string::npos);
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(usage.find("--target-steps"), std::string::npos);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(usage.find("Legacy positional"), std::string::npos);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(usage.find("--temperature"), std::string::npos);
}
/// Description: Executes the TEST operation.
TEST(ConfigArgsTest, TST_UNT_CONF_008_RejectsPositionalArguments)
{
    SimulationConfig config = SimulationConfig::defaults();
    RuntimeArgs runtime;
    std::stringstream warnings;
    const std::uint32_t initialParticleCount = config.particleCount;
    const float initialDt = config.dt;
    std::vector<std::string> args = {"app", "10000", "0.5"};
    const std::vector<std::string_view> argViews =
        /// Description: Executes the toArgViews operation.
        grav_test_config_args_cli_usage::toArgViews(args);
    /// Description: Executes the applyArgsToConfig operation.
    applyArgsToConfig(argViews, config, runtime, warnings);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(config.particleCount, initialParticleCount);
    /// Description: Executes the EXPECT_FLOAT_EQ operation.
    EXPECT_FLOAT_EQ(config.dt, initialDt);
    const std::string log = warnings.str();
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(runtime.hasArgumentError);
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(log.find("unexpected positional argument"), std::string::npos);
}
/// Description: Executes the TEST operation.
TEST(ConfigArgsTest, TST_UNT_CONF_009_RejectsLegacyTemperatureAlias)
{
    SimulationConfig config = SimulationConfig::defaults();
    RuntimeArgs runtime;
    std::stringstream warnings;
    const float initialVelocityTemperature = config.velocityTemperature;
    std::vector<std::string> args = {"app", "--temperature", "0.8"};
    const std::vector<std::string_view> argViews =
        /// Description: Executes the toArgViews operation.
        grav_test_config_args_cli_usage::toArgViews(args);
    /// Description: Executes the applyArgsToConfig operation.
    applyArgsToConfig(argViews, config, runtime, warnings);
    /// Description: Executes the EXPECT_FLOAT_EQ operation.
    EXPECT_FLOAT_EQ(config.velocityTemperature, initialVelocityTemperature);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(runtime.hasArgumentError);
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(warnings.str().find("unknown option: --temperature"), std::string::npos);
}
