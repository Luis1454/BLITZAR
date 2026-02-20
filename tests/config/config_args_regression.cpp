#include "sim/SimulationArgs.hpp"
#include "sim/SimulationConfig.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::vector<char *> toArgv(std::vector<std::string> &storage)
{
    std::vector<char *> argv;
    argv.reserve(storage.size());
    for (std::string &item : storage) {
        argv.push_back(item.data());
    }
    return argv;
}

} // namespace

TEST(ConfigArgsRegression, FindsConfigPathInline)
{
    std::vector<std::string> args = {"app", "--config=custom.ini"};
    std::vector<char *> argv = toArgv(args);
    EXPECT_EQ(findConfigPathArg(static_cast<int>(argv.size()), argv.data()), "custom.ini");
}

TEST(ConfigArgsRegression, FindsConfigPathSeparated)
{
    std::vector<std::string> args = {"app", "--config", "custom.ini"};
    std::vector<char *> argv = toArgv(args);
    EXPECT_EQ(findConfigPathArg(static_cast<int>(argv.size()), argv.data()), "custom.ini");
}

TEST(ConfigArgsRegression, AppliesValidArguments)
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
        "--energy-every", "2"
    };
    std::vector<char *> argv = toArgv(args);

    applyArgsToConfig(static_cast<int>(argv.size()), argv.data(), config, runtime, warnings);

    EXPECT_EQ(config.particleCount, 2048u);
    EXPECT_FLOAT_EQ(config.dt, 0.02f);
    EXPECT_EQ(config.solver, "octree_gpu");
    EXPECT_EQ(config.integrator, "rk4");
    EXPECT_TRUE(config.sphEnabled);
    EXPECT_EQ(config.uiFpsLimit, 75u);
    EXPECT_EQ(config.energyMeasureEverySteps, 2u);
    EXPECT_EQ(runtime.targetSteps, 333);
    EXPECT_FALSE(runtime.exportOnExit);
    EXPECT_TRUE(warnings.str().empty());
}

TEST(ConfigArgsRegression, RejectsInvalidArgumentsAndKeepsPreviousValues)
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
    std::vector<char *> argv = toArgv(args);

    applyArgsToConfig(static_cast<int>(argv.size()), argv.data(), config, runtime, warnings);

    EXPECT_EQ(config.particleCount, initialParticleCount);
    EXPECT_FLOAT_EQ(config.dt, initialDt);
    EXPECT_EQ(config.sphEnabled, initialSphEnabled);

    const std::string log = warnings.str();
    EXPECT_NE(log.find("invalid --particle-count"), std::string::npos);
    EXPECT_NE(log.find("invalid --dt"), std::string::npos);
    EXPECT_NE(log.find("invalid --sph"), std::string::npos);
    EXPECT_NE(log.find("unknown option"), std::string::npos);
}

TEST(ConfigArgsRegression, RejectsTrailingGarbageNumericArguments)
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
    std::vector<char *> argv = toArgv(args);

    applyArgsToConfig(static_cast<int>(argv.size()), argv.data(), config, runtime, warnings);

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

TEST(ConfigArgsRegression, SimulationConfigSaveLoadRoundTrip)
{
    SimulationConfig config = SimulationConfig::defaults();
    config.particleCount = 321u;
    config.dt = 0.015f;
    config.solver = "octree_cpu";
    config.integrator = "rk4";
    config.sphEnabled = true;
    config.exportFormat = "bin";
    config.energySampleLimit = 777u;

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

    std::error_code ec;
    std::filesystem::remove(path, ec);
}

TEST(ConfigArgsRegression, LoadIgnoresTrailingGarbageInNumericConfigValues)
{
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / ("gravity_config_invalid_numbers_" + std::to_string(stamp) + ".ini");

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

TEST(ConfigArgsRegression, LoadOrCreateCreatesFileWhenMissing)
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

TEST(ConfigArgsRegression, UsageIncludesCoreOptions)
{
    std::stringstream out;
    printUsage(out, "app", true);
    const std::string usage = out.str();
    EXPECT_NE(usage.find("--particle-count"), std::string::npos);
    EXPECT_NE(usage.find("--dt"), std::string::npos);
    EXPECT_NE(usage.find("--solver"), std::string::npos);
    EXPECT_NE(usage.find("--integrator"), std::string::npos);
    EXPECT_NE(usage.find("--target-steps"), std::string::npos);
}
