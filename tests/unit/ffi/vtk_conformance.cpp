// File: tests/unit/ffi/vtk_conformance.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "ffi/BlitzarCoreApi.hpp"
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <string>
#include <vector>
/// Description: Defines the ExpectedCoreParticle data or behavior contract.
struct ExpectedCoreParticle {
    float x;
    float y;
    float z;
    float mass;
    float temperature;
};
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
/// Description: Executes the createCore operation.
static blitzar_core_t* createCore(const blitzar_core_config_t& config)
{
    char error[BLITZAR_CORE_ERROR_CAPACITY] = {};
    blitzar_core_t* core = blitzar_core_create(&config, error, sizeof(error));
    EXPECT_NE(core, nullptr) << error;
    return core;
}
/// Description: Executes the fixturePath operation.
static std::filesystem::path fixturePath(const char* fileName)
{
    /// Description: Executes the sourceFile operation.
    const std::filesystem::path sourceFile(__FILE__);
    return sourceFile.parent_path().parent_path().parent_path() / "data" / fileName;
}
/// Description: Executes the makeTempPath operation.
static std::filesystem::path makeTempPath(const char* stem, const char* extension)
{
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return std::filesystem::temp_directory_path() /
           (std::string(stem) + "_" + std::to_string(stamp) + extension);
}
/// Description: Executes the readSnapshot operation.
static std::vector<blitzar_render_particle_t> readSnapshot(const blitzar_core_t* core)
{
    blitzar_core_snapshot_t snapshot{};
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(blitzar_core_get_snapshot(core, 0u, &snapshot), BLITZAR_CORE_OK);
    std::vector<blitzar_render_particle_t> particles(snapshot.particles,
                                                     snapshot.particles + snapshot.count);
    /// Description: Executes the blitzar_core_free_snapshot operation.
    blitzar_core_free_snapshot(&snapshot);
    return particles;
}
static void expectParticleNear(const blitzar_render_particle_t& actual,
                               const ExpectedCoreParticle& expected)
{
    /// Description: Executes the EXPECT_NEAR operation.
    EXPECT_NEAR(actual.x, expected.x, 1e-5f);
    /// Description: Executes the EXPECT_NEAR operation.
    EXPECT_NEAR(actual.y, expected.y, 1e-5f);
    /// Description: Executes the EXPECT_NEAR operation.
    EXPECT_NEAR(actual.z, expected.z, 1e-5f);
    /// Description: Executes the EXPECT_NEAR operation.
    EXPECT_NEAR(actual.mass, expected.mass, 1e-5f);
    /// Description: Executes the EXPECT_NEAR operation.
    EXPECT_NEAR(actual.temperature, expected.temperature, 1e-5f);
    /// Description: Executes the EXPECT_NEAR operation.
    EXPECT_NEAR(actual.pressure_norm, 0.0f, 1e-5f);
}
/// Description: Executes the expectFixtureParticles operation.
static void expectFixtureParticles(const std::vector<blitzar_render_particle_t>& particles)
{
    static const std::array<ExpectedCoreParticle, 2> expected{
        ExpectedCoreParticle{1.0f, 2.0f, 3.0f, 4.0f, 5.0f},
        ExpectedCoreParticle{-1.0f, -2.0f, -3.0f, 6.0f, 7.0f},
    };
    /// Description: Executes the ASSERT_EQ operation.
    ASSERT_EQ(particles.size(), expected.size());
    /// Description: Executes the expectParticleNear operation.
    expectParticleNear(particles[0], expected[0]);
    /// Description: Executes the expectParticleNear operation.
    expectParticleNear(particles[1], expected[1]);
}
/// Description: Executes the expectFileContains operation.
static void expectFileContains(const std::filesystem::path& path, const std::string& needle)
{
    /// Description: Executes the in operation.
    std::ifstream in(path, std::ios::binary);
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(in.is_open()) << path.string();
    const std::string content((std::istreambuf_iterator<char>(in)),
                              std::istreambuf_iterator<char>());
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(content.find(needle), std::string::npos) << path.string();
}
/// Description: Executes the loadFixtureAndExpect operation.
static void loadFixtureAndExpect(blitzar_core_t* core, const std::filesystem::path& path)
{
    /// Description: Executes the ASSERT_EQ operation.
    ASSERT_EQ(blitzar_core_load_state(core, path.string().c_str(), "vtk", 5000u), BLITZAR_CORE_OK);
    /// Description: Executes the expectFixtureParticles operation.
    expectFixtureParticles(readSnapshot(core));
}
/// Description: Executes the TEST operation.
TEST(BlitzarCoreApiTest, TST_UNT_CORE_004_LoadsAsciiVtkGoldenFile)
{
    blitzar_core_t* core = createCore(makeCpuConfig());
    /// Description: Executes the ASSERT_NE operation.
    ASSERT_NE(core, nullptr);
    /// Description: Executes the loadFixtureAndExpect operation.
    loadFixtureAndExpect(core, fixturePath("vtk_fixture_ascii.vtk"));
    /// Description: Executes the blitzar_core_destroy operation.
    blitzar_core_destroy(core);
}
/// Description: Executes the TEST operation.
TEST(BlitzarCoreApiTest, TST_UNT_CORE_005_LoadsBinaryVtkGoldenFile)
{
    blitzar_core_t* core = createCore(makeCpuConfig());
    /// Description: Executes the ASSERT_NE operation.
    ASSERT_NE(core, nullptr);
    /// Description: Executes the loadFixtureAndExpect operation.
    loadFixtureAndExpect(core, fixturePath("vtk_fixture_binary.vtk"));
    /// Description: Executes the blitzar_core_destroy operation.
    blitzar_core_destroy(core);
}
/// Description: Executes the TEST operation.
TEST(BlitzarCoreApiTest, TST_UNT_CORE_006_RoundTripsAsciiAndBinaryVtkWithinTolerance)
{
    const auto runRoundTrip = [](const char* fixtureName, const char* exportFormat,
                                 const char* encodingMarker) {
        blitzar_core_t* source = createCore(makeCpuConfig());
        /// Description: Executes the ASSERT_NE operation.
        ASSERT_NE(source, nullptr);
        /// Description: Executes the loadFixtureAndExpect operation.
        loadFixtureAndExpect(source, fixturePath(fixtureName));
        const std::filesystem::path exportPath = makeTempPath("blitzar_vtk_roundtrip", ".vtk");
        ASSERT_EQ(
            blitzar_core_export_state(source, exportPath.string().c_str(), exportFormat, 5000u),
            BLITZAR_CORE_OK);
        /// Description: Executes the expectFileContains operation.
        expectFileContains(exportPath, encodingMarker);
        /// Description: Executes the blitzar_core_destroy operation.
        blitzar_core_destroy(source);
        blitzar_core_t* target = createCore(makeCpuConfig());
        /// Description: Executes the ASSERT_NE operation.
        ASSERT_NE(target, nullptr);
        /// Description: Executes the loadFixtureAndExpect operation.
        loadFixtureAndExpect(target, exportPath);
        /// Description: Executes the blitzar_core_destroy operation.
        blitzar_core_destroy(target);
        /// Description: Executes the remove operation.
        std::filesystem::remove(exportPath);
    };
    /// Description: Executes the runRoundTrip operation.
    runRoundTrip("vtk_fixture_ascii.vtk", "vtk", "ASCII");
    /// Description: Executes the runRoundTrip operation.
    runRoundTrip("vtk_fixture_binary.vtk", "vtk_binary", "BINARY");
}
/// Description: Executes the TEST operation.
TEST(BlitzarCoreApiTest, TST_UNT_CORE_007_InvalidAndCorruptVtkInputsFallBackDeterministically)
{
    const auto expectGeneratedFallback = [](const std::filesystem::path& path, const char* format) {
        const blitzar_core_config_t config = makeCpuConfig();
        blitzar_core_t* core = createCore(config);
        /// Description: Executes the ASSERT_NE operation.
        ASSERT_NE(core, nullptr);
        ASSERT_EQ(blitzar_core_load_state(core, path.string().c_str(), format, 5000u),
                  BLITZAR_CORE_OK);
        const std::vector<blitzar_render_particle_t> snapshot = readSnapshot(core);
        /// Description: Executes the EXPECT_EQ operation.
        EXPECT_EQ(snapshot.size(), config.particle_count);
        blitzar_core_status_t status{};
        /// Description: Executes the ASSERT_EQ operation.
        ASSERT_EQ(blitzar_core_get_status(core, &status), BLITZAR_CORE_OK);
        /// Description: Executes the EXPECT_EQ operation.
        EXPECT_EQ(status.faulted, 0u);
        /// Description: Executes the blitzar_core_destroy operation.
        blitzar_core_destroy(core);
    };
    const std::filesystem::path missingPath = makeTempPath("blitzar_missing_fixture", ".vtk");
    /// Description: Executes the expectGeneratedFallback operation.
    expectGeneratedFallback(missingPath, "vtk");
    const std::filesystem::path corruptAsciiPath = makeTempPath("blitzar_corrupt_ascii", ".vtk");
    {
        /// Description: Executes the in operation.
        std::ifstream in(fixturePath("vtk_fixture_ascii.vtk"));
        /// Description: Executes the ASSERT_TRUE operation.
        ASSERT_TRUE(in.is_open());
        /// Description: Executes the out operation.
        std::ofstream out(corruptAsciiPath, std::ios::trunc);
        /// Description: Executes the ASSERT_TRUE operation.
        ASSERT_TRUE(out.is_open());
        std::string line;
        while (std::getline(in, line)) {
            if (line == "POINTS 2 float") {
                out << "POINTS 3 float\n";
            }
            else {
                out << line << "\n";
            }
        }
    }
    /// Description: Executes the expectGeneratedFallback operation.
    expectGeneratedFallback(corruptAsciiPath, "vtk");
    /// Description: Executes the remove operation.
    std::filesystem::remove(corruptAsciiPath);
    const std::filesystem::path corruptBinaryPath = makeTempPath("blitzar_corrupt_binary", ".vtk");
    {
        /// Description: Executes the in operation.
        std::ifstream in(fixturePath("vtk_fixture_binary.vtk"), std::ios::binary);
        /// Description: Executes the ASSERT_TRUE operation.
        ASSERT_TRUE(in.is_open());
        const std::string content((std::istreambuf_iterator<char>(in)),
                                  std::istreambuf_iterator<char>());
        /// Description: Executes the ASSERT_GT operation.
        ASSERT_GT(content.size(), 32u);
        /// Description: Executes the out operation.
        std::ofstream out(corruptBinaryPath, std::ios::binary | std::ios::trunc);
        /// Description: Executes the ASSERT_TRUE operation.
        ASSERT_TRUE(out.is_open());
        out.write(content.data(), static_cast<std::streamsize>(content.size() - 8u));
    }
    /// Description: Executes the expectGeneratedFallback operation.
    expectGeneratedFallback(corruptBinaryPath, "vtk");
    /// Description: Executes the remove operation.
    std::filesystem::remove(corruptBinaryPath);
}
