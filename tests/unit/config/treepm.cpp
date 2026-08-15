/*
 * @file tests/unit/config/treepm.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Configuration coverage for selectable TreePM execution models.
 */

#include "config/args/Main.hpp"
#include "config/core/Config.hpp"

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

static std::vector<std::string_view> toArgViews(const std::vector<std::string>& storage)
{
    std::vector<std::string_view> args;
    args.reserve(storage.size());
    for (const std::string& item : storage) {
        args.emplace_back(item);
    }
    return args;
}
TEST(TreePmConfigTest, TST_UNT_CONF_094_CliPresetSelectsHybridModel)
{
    SimulationConfig config = SimulationConfig::defaults();
    RuntimeArgs runtime;
    std::stringstream warnings;
    const std::vector<std::string> args{"app", "--treepm-preset", "hybrid_quality"};

    applyArgsToConfig(toArgViews(args), config, runtime, warnings);

    EXPECT_FALSE(runtime.hasArgumentError);
    EXPECT_TRUE(warnings.str().empty());
    EXPECT_TRUE(config.treePmEnabled);
    EXPECT_EQ(config.treePmPreset, "hybrid_quality");
    EXPECT_EQ(config.treePmModel, "hybrid");
    EXPECT_EQ(config.treePmDenseCellThreshold, 32u);
    EXPECT_EQ(config.treePmMaxLocalNeighbors, 128u);
}

TEST(TreePmConfigTest, TST_UNT_CONF_095_CliKeepsCustomPmLimits)
{
    SimulationConfig config = SimulationConfig::defaults();
    RuntimeArgs runtime;
    std::stringstream warnings;
    const std::vector<std::string> args{"app",    "--treepm-model",
                                        "hybrid", "--treepm-particle-limit",
                                        "4096",   "--treepm-dense-cell-threshold",
                                        "12"};

    applyArgsToConfig(toArgViews(args), config, runtime, warnings);

    EXPECT_FALSE(runtime.hasArgumentError);
    EXPECT_EQ(config.treePmModel, "hybrid");
    EXPECT_EQ(config.treePmParticleLimit, 4096u);
    EXPECT_EQ(config.treePmDenseCellThreshold, 12u);
}

TEST(TreePmConfigTest, TST_UNT_CONF_096_DirectiveAppliesPresetBeforeOverrides)
{
    const std::string path = "treepm-config-test.ini";
    {
        std::ofstream out(path, std::ios::trunc);
        ASSERT_TRUE(out.is_open());
        out << "treepm(preset=hybrid_balanced, particle_limit=1024, "
               "dense_cell_threshold=10, model=hybrid)\n";
    }

    const SimulationConfig loaded = SimulationConfig::loadOrCreate(path);

    EXPECT_EQ(loaded.treePmPreset, "hybrid_balanced");
    EXPECT_EQ(loaded.treePmModel, "hybrid");
    EXPECT_EQ(loaded.treePmParticleLimit, 1024u);
    EXPECT_EQ(loaded.treePmDenseCellThreshold, 10u);
    std::error_code ec;
    std::filesystem::remove(path, ec);
}

TEST(TreePmConfigTest, TST_UNT_CONF_097_QualityPresetSelectsExactGpuTree)
{
    SimulationConfig config = SimulationConfig::defaults();
    RuntimeArgs runtime;
    std::stringstream warnings;
    const std::vector<std::string> args{"app", "--treepm-preset", "tree_quality"};

    applyArgsToConfig(toArgViews(args), config, runtime, warnings);

    EXPECT_FALSE(runtime.hasArgumentError);
    EXPECT_TRUE(warnings.str().empty());
    EXPECT_TRUE(config.treePmEnabled);
    EXPECT_EQ(config.treePmModel, "exact_tree");
}

TEST(TreePmConfigTest, TST_UNT_CONF_098_CliAndDirectiveSelectFp64Precision)
{
    SimulationConfig config = SimulationConfig::defaults();
    RuntimeArgs runtime;
    std::stringstream warnings;
    const std::vector<std::string> args{"app", "--treepm-precision", "fp64"};

    applyArgsToConfig(toArgViews(args), config, runtime, warnings);

    EXPECT_FALSE(runtime.hasArgumentError);
    EXPECT_TRUE(warnings.str().empty());
    EXPECT_EQ(config.treePmPrecision, "fp64");

    const std::string path = "treepm-precision-config-test.ini";
    {
        std::ofstream out(path, std::ios::trunc);
        ASSERT_TRUE(out.is_open());
        out << "treepm(enabled=true, model=pm_only, precision=fp64)\n";
    }
    const SimulationConfig loaded = SimulationConfig::loadOrCreate(path);
    EXPECT_EQ(loaded.treePmPrecision, "fp64");
    std::error_code ec;
    std::filesystem::remove(path, ec);
}

TEST(TreePmConfigTest, TST_UNT_CONF_099_CliAndDirectiveSelectAssignmentStencil)
{
    SimulationConfig config = SimulationConfig::defaults();
    RuntimeArgs runtime;
    std::stringstream warnings;
    const std::vector<std::string> args{"app", "--treepm-assignment", "pcs"};

    applyArgsToConfig(toArgViews(args), config, runtime, warnings);

    EXPECT_FALSE(runtime.hasArgumentError);
    EXPECT_TRUE(warnings.str().empty());
    EXPECT_EQ(config.treePmAssignment, "pcs");

    const std::string path = "treepm-assignment-config-test.ini";
    {
        std::ofstream out(path, std::ios::trunc);
        ASSERT_TRUE(out.is_open());
        out << "treepm(enabled=true, model=pm_only, assignment=tsc)\n";
    }
    const SimulationConfig loaded = SimulationConfig::loadOrCreate(path);
    EXPECT_EQ(loaded.treePmAssignment, "tsc");
    std::error_code ec;
    std::filesystem::remove(path, ec);
}

TEST(TreePmConfigTest, TST_UNT_CONF_112_CliAndDirectiveSelectParticleLayout)
{
    SimulationConfig config = SimulationConfig::defaults();
    RuntimeArgs runtime;
    std::stringstream warnings;
    const std::vector<std::string> args{"app", "--treepm-layout", "gather_morton"};

    applyArgsToConfig(toArgViews(args), config, runtime, warnings);

    EXPECT_FALSE(runtime.hasArgumentError);
    EXPECT_TRUE(warnings.str().empty());
    EXPECT_EQ(config.treePmLayout, "gather_morton");

    const std::string path = "treepm-layout-config-test.ini";
    {
        std::ofstream out(path, std::ios::trunc);
        ASSERT_TRUE(out.is_open());
        out << "treepm(enabled=true, layout=gather_linear)\n";
    }
    const SimulationConfig loaded = SimulationConfig::loadOrCreate(path);
    EXPECT_EQ(loaded.treePmLayout, "gather_linear");
    std::error_code ec;
    std::filesystem::remove(path, ec);
}

TEST(AdaptiveTimeStepConfigTest, TST_UNT_CONF_100_CliSelectsDyadicHierarchy)
{
    SimulationConfig config = SimulationConfig::defaults();
    RuntimeArgs runtime;
    std::stringstream warnings;
    const std::vector<std::string> args{"app", "--adaptive-time-steps", "true",
                                        "--adaptive-max-level", "8", "--adaptive-eta", "0.2"};

    applyArgsToConfig(toArgViews(args), config, runtime, warnings);

    EXPECT_FALSE(runtime.hasArgumentError);
    EXPECT_TRUE(warnings.str().empty());
    EXPECT_TRUE(config.adaptiveTimeStepsEnabled);
    EXPECT_EQ(config.adaptiveTimeStepMaxLevel, 8u);
    EXPECT_FLOAT_EQ(config.adaptiveTimeStepEta, 0.2f);
}

TEST(AdaptiveTimeStepConfigTest, TST_UNT_CONF_101_DirectiveRoundTripsAdaptiveSettings)
{
    const std::string path = "adaptive-config-test.ini";
    {
        std::ofstream out(path, std::ios::trunc);
        ASSERT_TRUE(out.is_open());
        out << "adaptive(enabled=true, max_level=6, eta=0.15)\n";
    }
    const SimulationConfig loaded = SimulationConfig::loadOrCreate(path);
    EXPECT_TRUE(loaded.adaptiveTimeStepsEnabled);
    EXPECT_EQ(loaded.adaptiveTimeStepMaxLevel, 6u);
    EXPECT_FLOAT_EQ(loaded.adaptiveTimeStepEta, 0.15f);
    std::error_code ec;
    std::filesystem::remove(path, ec);
}
