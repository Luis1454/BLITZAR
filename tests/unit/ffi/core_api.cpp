// File: tests/unit/ffi/core_api.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "ffi/BlitzarCoreApi.hpp"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <gtest/gtest.h>
#include <string>

/// Description: Executes the makeCpuConfig operation.
static blitzar_core_config_t makeCpuConfig()
{
    blitzar_core_config_t config = blitzar_core_default_config();
    config.particle_count = 24u;
    config.dt = 0.01f;
    config.solver_name = "octree_cpu";
    config.integrator_name = "euler";
    config.performance_profile = "interactive";
    config.substep_target_dt = 0.01f;
    config.max_substeps = 1u;
    config.snapshot_publish_period_ms = 5u;
    return config;
}

/// Description: Executes the makeTempPath operation.
static std::filesystem::path makeTempPath(const char* stem, const char* extension)
{
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return std::filesystem::temp_directory_path() /
           (std::string(stem) + "_" + std::to_string(stamp) + extension);
}

/// Description: Executes the createCore operation.
static blitzar_core_t* createCore(const blitzar_core_config_t& config)
{
    char error[BLITZAR_CORE_ERROR_CAPACITY] = {};
    blitzar_core_t* core = blitzar_core_create(&config, error, sizeof(error));
    EXPECT_NE(core, nullptr) << error;
    return core;
}

/// Description: Executes the TEST operation.
TEST(BlitzarCoreApiTest, TST_UNT_CORE_001_CreateRunAndReportStatus)
{
    blitzar_core_t* core = createCore(makeCpuConfig());
    ASSERT_NE(core, nullptr);
    blitzar_core_status_t status{};
    EXPECT_EQ(blitzar_core_get_status(core, &status), BLITZAR_CORE_OK);
    EXPECT_EQ(status.paused, 1u);
    EXPECT_EQ(status.steps, 0u);
    EXPECT_STREQ(status.solver_name, "octree_cpu");
    EXPECT_STREQ(status.integrator_name, "euler");
    EXPECT_EQ(blitzar_core_run_steps(core, 3u, 5000u), BLITZAR_CORE_OK);
    EXPECT_EQ(blitzar_core_get_status(core, &status), BLITZAR_CORE_OK);
    EXPECT_EQ(status.paused, 1u);
    EXPECT_EQ(status.steps, 3u);
    EXPECT_EQ(status.faulted, 0u);
    blitzar_core_destroy(core);
}

/// Description: Executes the TEST operation.
TEST(BlitzarCoreApiTest, TST_UNT_CORE_002_CopiesAndFreesSnapshotBuffer)
{
    blitzar_core_t* core = createCore(makeCpuConfig());
    ASSERT_NE(core, nullptr);
    ASSERT_EQ(blitzar_core_run_steps(core, 1u, 5000u), BLITZAR_CORE_OK);
    blitzar_core_snapshot_t snapshot{};
    EXPECT_EQ(blitzar_core_get_snapshot(core, 0u, &snapshot), BLITZAR_CORE_OK);
    ASSERT_NE(snapshot.particles, nullptr);
    EXPECT_GE(snapshot.count, 24u);
    EXPECT_TRUE(std::any_of(snapshot.particles, snapshot.particles + snapshot.count,
                            [](const blitzar_render_particle_t& particle) {
                                return particle.mass > 0.0f;
                            }));
    blitzar_core_free_snapshot(&snapshot);
    EXPECT_EQ(snapshot.particles, nullptr);
    EXPECT_EQ(snapshot.count, 0u);
    blitzar_core_destroy(core);
}

/// Description: Executes the TEST operation.
TEST(BlitzarCoreApiTest, TST_UNT_CORE_003_ExportsAndReloadsState)
{
    const std::filesystem::path exportPath = makeTempPath("blitzar_core_export", ".vtk");
    const blitzar_core_config_t config = makeCpuConfig();
    blitzar_core_t* source = createCore(config);
    ASSERT_NE(source, nullptr);
    ASSERT_EQ(blitzar_core_run_steps(source, 2u, 5000u), BLITZAR_CORE_OK);
    ASSERT_EQ(blitzar_core_export_state(source, exportPath.string().c_str(), "vtk", 5000u),
              BLITZAR_CORE_OK);
    ASSERT_TRUE(std::filesystem::exists(exportPath));
    blitzar_core_destroy(source);
    blitzar_core_t* target = createCore(config);
    ASSERT_NE(target, nullptr);
    ASSERT_EQ(blitzar_core_load_state(target, exportPath.string().c_str(), "vtk", 5000u),
              BLITZAR_CORE_OK);
    blitzar_core_status_t status{};
    ASSERT_EQ(blitzar_core_get_status(target, &status), BLITZAR_CORE_OK);
    EXPECT_EQ(status.steps, 0u);
    EXPECT_EQ(status.faulted, 0u);
    blitzar_core_snapshot_t snapshot{};
    ASSERT_EQ(blitzar_core_get_snapshot(target, 0u, &snapshot), BLITZAR_CORE_OK);
    EXPECT_EQ(snapshot.count, 24u);
    blitzar_core_free_snapshot(&snapshot);
    blitzar_core_destroy(target);
    std::filesystem::remove(exportPath);
}
