#include "config/SimulationArgs.hpp"
#include "config/SimulationConfig.hpp"
#include "protocol/ServerProtocol.hpp"

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
    EXPECT_EQ(findConfigPathArg(grav_test_config_args_cli::toArgViews(args)), "custom.ini");
}

TEST(ConfigArgsTest, TST_UNT_CONF_002_FindsConfigPathSeparated)
{
    std::vector<std::string> args = {"app", "--config", "custom.ini"};
    EXPECT_EQ(findConfigPathArg(grav_test_config_args_cli::toArgViews(args)), "custom.ini");
}

TEST(ConfigArgsTest, TST_UNT_CONF_003_AppliesValidArguments)
{
    SimulationConfig config = SimulationConfig::defaults();
    RuntimeArgs runtime;
    std::stringstream warnings;
    std::vector<std::string> args = {"app", "--particle-count", "2048", "--dt=0.02", "--solver", "octree_gpu", "--integrator", "euler",
        "--sph", "true", "--target-steps", "333", "--export-on-exit=false", "--ui-fps", "75", "--energy-every", "2",
        "--server-command-timeout-ms", "90", "--server-status-timeout-ms", "35", "--server-snapshot-timeout-ms", "180"};
    applyArgsToConfig(grav_test_config_args_cli::toArgViews(args), config, runtime, warnings);
    EXPECT_EQ(config.particleCount, 2048u);
    EXPECT_FLOAT_EQ(config.dt, 0.02f);
    EXPECT_EQ(config.solver, "octree_gpu");
    EXPECT_EQ(config.integrator, "euler");
    EXPECT_TRUE(config.sphEnabled);
    EXPECT_EQ(config.uiFpsLimit, 75u);
    EXPECT_EQ(config.energyMeasureEverySteps, 2u);
    EXPECT_EQ(config.clientRemoteCommandTimeoutMs, 90u);
    EXPECT_EQ(config.clientRemoteStatusTimeoutMs, 35u);
    EXPECT_EQ(config.clientRemoteSnapshotTimeoutMs, 180u);
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
    std::vector<std::string> args = {"app", "--particle-count", "nope", "--dt", "-1", "--sph", "maybe", "--unknown", "value"};
    applyArgsToConfig(grav_test_config_args_cli::toArgViews(args), config, runtime, warnings);
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
    std::vector<std::string> args = {"app", "--solver", "bad_solver", "--integrator", "bad_integrator"};
    applyArgsToConfig(grav_test_config_args_cli::toArgViews(args), config, runtime, warnings);
    EXPECT_EQ(config.solver, initialSolver);
    EXPECT_EQ(config.integrator, initialIntegrator);
    EXPECT_TRUE(runtime.hasArgumentError);
    const std::string log = warnings.str();
    EXPECT_NE(log.find("invalid --solver"), std::string::npos);
    EXPECT_NE(log.find("invalid --integrator"), std::string::npos);

    runtime = RuntimeArgs{};
    warnings.str("");
    warnings.clear();
    args = {"app", "--solver", "octree_gpu", "--integrator", "rk4"};
    applyArgsToConfig(grav_test_config_args_cli::toArgViews(args), config, runtime, warnings);
    EXPECT_EQ(config.solver, initialSolver);
    EXPECT_EQ(config.integrator, initialIntegrator);
    EXPECT_TRUE(runtime.hasArgumentError);
    EXPECT_NE(warnings.str().find("unsupported solver/integrator combination"), std::string::npos);
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
    std::vector<std::string> args = {"app", "--particle-count", "2048abc", "--dt", "0.01x", "--octree-theta", "1.4deg", "--luminosity", "120%"};
    applyArgsToConfig(grav_test_config_args_cli::toArgViews(args), config, runtime, warnings);
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

TEST(ConfigArgsTest, TST_UNT_CONF_014_ClampsClientParticleCapArgumentToProtocolMax)
{
    SimulationConfig config = SimulationConfig::defaults();
    RuntimeArgs runtime;
    std::stringstream warnings;
    std::vector<std::string> args = {"app", "--client-particle-cap", "50000"};
    applyArgsToConfig(grav_test_config_args_cli::toArgViews(args), config, runtime, warnings);
    EXPECT_EQ(config.clientParticleCap, grav_protocol::kSnapshotMaxPoints);
    EXPECT_NE(warnings.str().find("--client-particle-cap clamped"), std::string::npos);
    EXPECT_FALSE(runtime.hasArgumentError);
}

TEST(ConfigArgsTest, TST_UNT_CONF_019_CliAliasesApplyThroughSharedRegistry)
{
    SimulationConfig config = SimulationConfig::defaults();
    RuntimeArgs runtime;
    std::stringstream warnings;
    std::vector<std::string> args = {"app", "--structure", "random_cloud", "--size", "24"};
    applyArgsToConfig(grav_test_config_args_cli::toArgViews(args), config, runtime, warnings);
    EXPECT_EQ(config.presetStructure, "random_cloud");
    EXPECT_FLOAT_EQ(config.presetSize, 24.0f);
    EXPECT_TRUE(warnings.str().empty());
    EXPECT_FALSE(runtime.hasArgumentError);
}

TEST(ConfigArgsTest, TST_UNT_CONF_022_CliAcceptsCalibrationSceneModes)
{
    SimulationConfig config = SimulationConfig::defaults();
    RuntimeArgs runtime;
    std::stringstream warnings;
    std::vector<std::string> args = {"app", "--preset-structure", "three_body", "--init-mode", "plummer_sphere"};
    applyArgsToConfig(grav_test_config_args_cli::toArgViews(args), config, runtime, warnings);
    EXPECT_EQ(config.presetStructure, "three_body");
    EXPECT_EQ(config.initMode, "plummer_sphere");
    EXPECT_TRUE(warnings.str().empty());
    EXPECT_FALSE(runtime.hasArgumentError);
}
