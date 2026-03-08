#include "config/SimulationArgs.hpp"
#include "config/SimulationConfig.hpp"
#include "protocol/BackendProtocol.hpp"

#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace grav_test_config_args_cli {

std::vector<std::string_view> toArgViews(const std::vector<std::string> &storage)
{
    std::vector<std::string_view> args;
    args.reserve(storage.size());
    for (const std::string &item : storage) {
        args.emplace_back(item);
    }
    return args;
}

} // namespace grav_test_config_args_cli

TEST(ConfigArgsTest, TST_UNT_CONF_001_FindsConfigPathInline)
{
    std::vector<std::string> args = {"app", "--config=custom.ini"};
    const std::vector<std::string_view> argViews = grav_test_config_args_cli::toArgViews(args);
    EXPECT_EQ(findConfigPathArg(argViews), "custom.ini");
}

TEST(ConfigArgsTest, TST_UNT_CONF_002_FindsConfigPathSeparated)
{
    std::vector<std::string> args = {"app", "--config", "custom.ini"};
    const std::vector<std::string_view> argViews = grav_test_config_args_cli::toArgViews(args);
    EXPECT_EQ(findConfigPathArg(argViews), "custom.ini");
}

TEST(ConfigArgsTest, TST_UNT_CONF_003_AppliesValidArguments)
{
    SimulationConfig config = SimulationConfig::defaults();
    RuntimeArgs runtime;
    std::stringstream warnings;

    std::vector<std::string> args = {
        "app",
        "--particle-count", "2048",
        "--dt=0.02",
        "--solver", "octree_gpu",
        "--integrator", "rk4",
        "--sph", "true",
        "--target-steps", "333",
        "--export-on-exit=false",
        "--ui-fps", "75",
        "--energy-every", "2",
        "--backend-command-timeout-ms", "90",
        "--backend-status-timeout-ms", "35",
        "--backend-snapshot-timeout-ms", "180"
    };
    const std::vector<std::string_view> argViews = grav_test_config_args_cli::toArgViews(args);
    applyArgsToConfig(argViews, config, runtime, warnings);

    EXPECT_EQ(config.particleCount, 2048u);
    EXPECT_FLOAT_EQ(config.dt, 0.02f);
    EXPECT_EQ(config.solver, "octree_gpu");
    EXPECT_EQ(config.integrator, "rk4");
    EXPECT_TRUE(config.sphEnabled);
    EXPECT_EQ(config.uiFpsLimit, 75u);
    EXPECT_EQ(config.energyMeasureEverySteps, 2u);
    EXPECT_EQ(config.frontendRemoteCommandTimeoutMs, 90u);
    EXPECT_EQ(config.frontendRemoteStatusTimeoutMs, 35u);
    EXPECT_EQ(config.frontendRemoteSnapshotTimeoutMs, 180u);
    EXPECT_EQ(runtime.targetSteps, 333);
    EXPECT_FALSE(runtime.exportOnExit);
    EXPECT_TRUE(warnings.str().empty());
}

TEST(ConfigArgsTest, TST_UNT_CONF_004_RejectsInvalidArgumentsAndKeepsPreviousValues)
{
    SimulationConfig config = SimulationConfig::defaults();
    RuntimeArgs runtime;
    std::stringstream warnings;

    const std::uint32_t initialParticleCount = config.particleCount;
    const float initialDt = config.dt;
    const bool initialSphEnabled = config.sphEnabled;

    std::vector<std::string> args = {
        "app",
        "--particle-count", "nope",
        "--dt", "-1",
        "--sph", "maybe",
        "--unknown", "value"
    };
    const std::vector<std::string_view> argViews = grav_test_config_args_cli::toArgViews(args);
    applyArgsToConfig(argViews, config, runtime, warnings);

    EXPECT_EQ(config.particleCount, initialParticleCount);
    EXPECT_FLOAT_EQ(config.dt, initialDt);
    EXPECT_EQ(config.sphEnabled, initialSphEnabled);

    const std::string log = warnings.str();
    EXPECT_NE(log.find("invalid --particle-count"), std::string::npos);
    EXPECT_NE(log.find("invalid --dt"), std::string::npos);
    EXPECT_NE(log.find("invalid --sph"), std::string::npos);
    EXPECT_NE(log.find("unknown option"), std::string::npos);
}

TEST(ConfigArgsTest, TST_UNT_CONF_005_RejectsInvalidSolverAndIntegratorValues)
{
    SimulationConfig config = SimulationConfig::defaults();
    RuntimeArgs runtime;
    std::stringstream warnings;

    const std::string initialSolver = config.solver;
    const std::string initialIntegrator = config.integrator;

    std::vector<std::string> args = {
        "app",
        "--solver", "bad_solver",
        "--integrator", "bad_integrator"
    };
    const std::vector<std::string_view> argViews = grav_test_config_args_cli::toArgViews(args);
    applyArgsToConfig(argViews, config, runtime, warnings);

    EXPECT_EQ(config.solver, initialSolver);
    EXPECT_EQ(config.integrator, initialIntegrator);
    EXPECT_TRUE(runtime.hasArgumentError);
    const std::string log = warnings.str();
    EXPECT_NE(log.find("invalid --solver"), std::string::npos);
    EXPECT_NE(log.find("invalid --integrator"), std::string::npos);
}

TEST(ConfigArgsTest, TST_UNT_CONF_006_RejectsTrailingGarbageNumericArguments)
{
    SimulationConfig config = SimulationConfig::defaults();
    RuntimeArgs runtime;
    std::stringstream warnings;

    const std::uint32_t initialParticleCount = config.particleCount;
    const float initialDt = config.dt;
    const float initialTheta = config.octreeTheta;
    const int initialLuminosity = config.defaultLuminosity;

    std::vector<std::string> args = {
        "app",
        "--particle-count", "2048abc",
        "--dt", "0.01x",
        "--octree-theta", "1.4deg",
        "--luminosity", "120%"
    };
    const std::vector<std::string_view> argViews = grav_test_config_args_cli::toArgViews(args);
    applyArgsToConfig(argViews, config, runtime, warnings);

    EXPECT_EQ(config.particleCount, initialParticleCount);
    EXPECT_FLOAT_EQ(config.dt, initialDt);
    EXPECT_FLOAT_EQ(config.octreeTheta, initialTheta);
    EXPECT_EQ(config.defaultLuminosity, initialLuminosity);

    const std::string log = warnings.str();
    EXPECT_NE(log.find("invalid --particle-count"), std::string::npos);
    EXPECT_NE(log.find("invalid --dt"), std::string::npos);
    EXPECT_NE(log.find("invalid --octree-theta"), std::string::npos);
    EXPECT_NE(log.find("invalid --luminosity"), std::string::npos);
}

TEST(ConfigArgsTest, TST_UNT_CONF_014_ClampsFrontendParticleCapArgumentToProtocolMax)
{
    SimulationConfig config = SimulationConfig::defaults();
    RuntimeArgs runtime;
    std::stringstream warnings;

    std::vector<std::string> args = {"app", "--frontend-particle-cap", "50000"};
    const std::vector<std::string_view> argViews = grav_test_config_args_cli::toArgViews(args);
    applyArgsToConfig(argViews, config, runtime, warnings);

    EXPECT_EQ(config.frontendParticleCap, grav_protocol::kSnapshotMaxPoints);
    EXPECT_NE(warnings.str().find("--frontend-particle-cap clamped"), std::string::npos);
    EXPECT_FALSE(runtime.hasArgumentError);
}

