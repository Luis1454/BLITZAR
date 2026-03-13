#include "config/SimulationArgs.hpp"
#include "config/SimulationConfig.hpp"

#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace grav_test_config_args_cli_usage {

std::vector<std::string_view> toArgViews(const std::vector<std::string> &storage)
{
    std::vector<std::string_view> args;
    args.reserve(storage.size());
    for (const std::string &item : storage) {
        args.emplace_back(item);
    }
    return args;
}

} // namespace grav_test_config_args_cli_usage

TEST(ConfigArgsTest, TST_UNT_CONF_007_UsageIncludesCoreOptions)
{
    std::stringstream out;
    printUsage(out, "app", true);
    const std::string usage = out.str();
    EXPECT_NE(usage.find("--particle-count"), std::string::npos);
    EXPECT_NE(usage.find("--dt"), std::string::npos);
    EXPECT_NE(usage.find("--solver"), std::string::npos);
    EXPECT_NE(usage.find("--integrator"), std::string::npos);
    EXPECT_NE(usage.find("--server-command-timeout-ms"), std::string::npos);
    EXPECT_NE(usage.find("--target-steps"), std::string::npos);
    EXPECT_EQ(usage.find("Legacy positional"), std::string::npos);
    EXPECT_EQ(usage.find("--temperature"), std::string::npos);
}

TEST(ConfigArgsTest, TST_UNT_CONF_008_RejectsPositionalArguments)
{
    SimulationConfig config = SimulationConfig::defaults();
    RuntimeArgs runtime;
    std::stringstream warnings;

    const std::uint32_t initialParticleCount = config.particleCount;
    const float initialDt = config.dt;

    std::vector<std::string> args = {"app", "10000", "0.5"};
    const std::vector<std::string_view> argViews = grav_test_config_args_cli_usage::toArgViews(args);
    applyArgsToConfig(argViews, config, runtime, warnings);

    EXPECT_EQ(config.particleCount, initialParticleCount);
    EXPECT_FLOAT_EQ(config.dt, initialDt);

    const std::string log = warnings.str();
    EXPECT_TRUE(runtime.hasArgumentError);
    EXPECT_NE(log.find("unexpected positional argument"), std::string::npos);
}

TEST(ConfigArgsTest, TST_UNT_CONF_009_RejectsLegacyTemperatureAlias)
{
    SimulationConfig config = SimulationConfig::defaults();
    RuntimeArgs runtime;
    std::stringstream warnings;

    const float initialVelocityTemperature = config.velocityTemperature;

    std::vector<std::string> args = {"app", "--temperature", "0.8"};
    const std::vector<std::string_view> argViews = grav_test_config_args_cli_usage::toArgViews(args);
    applyArgsToConfig(argViews, config, runtime, warnings);

    EXPECT_FLOAT_EQ(config.velocityTemperature, initialVelocityTemperature);
    EXPECT_TRUE(runtime.hasArgumentError);
    EXPECT_NE(warnings.str().find("unknown option: --temperature"), std::string::npos);
}
