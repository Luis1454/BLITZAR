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
    EXPECT_EQ(blitzar_core_get_snapshot(core, 0u, &snapshot), BLITZAR_CORE_OK);
    std::vector<blitzar_render_particle_t> particles(snapshot.particles,
                                                     snapshot.particles + snapshot.count);
    blitzar_core_free_snapshot(&snapshot);
    return particles;
}

/// Description: Describes the expect particle near operation contract.
static void expectParticleNear(const blitzar_render_particle_t& actual,
                               const ExpectedCoreParticle& expected)
{
    EXPECT_NEAR(actual.x, expected.x, 1e-5f);
    EXPECT_NEAR(actual.y, expected.y, 1e-5f);
    EXPECT_NEAR(actual.z, expected.z, 1e-5f);
    EXPECT_NEAR(actual.mass, expected.mass, 1e-5f);
    EXPECT_NEAR(actual.temperature, expected.temperature, 1e-5f);
    EXPECT_NEAR(actual.pressure_norm, 0.0f, 1e-5f);
}

/// Description: Executes the expectFixtureParticles operation.
static void expectFixtureParticles(const std::vector<blitzar_render_particle_t>& particles)
{
    static const std::array<ExpectedCoreParticle, 2> expected{
        ExpectedCoreParticle{1.0f, 2.0f, 3.0f, 4.0f, 5.0f},
        ExpectedCoreParticle{-1.0f, -2.0f, -3.0f, 6.0f, 7.0f},
    };
    ASSERT_EQ(particles.size(), expected.size());
    expectParticleNear(particles[0], expected[0]);
    expectParticleNear(particles[1], expected[1]);
}

/// Description: Executes the expectFileContains operation.
static void expectFileContains(const std::filesystem::path& path, const std::string& needle)
{
    std::ifstream in(path, std::ios::binary);
    ASSERT_TRUE(in.is_open()) << path.string();
    const std::string content((std::istreambuf_iterator<char>(in)),
                              std::istreambuf_iterator<char>());
    EXPECT_NE(content.find(needle), std::string::npos) << path.string();
}

/// Description: Executes the loadFixtureAndExpect operation.
static void loadFixtureAndExpect(blitzar_core_t* core, const std::filesystem::path& path)
{
    ASSERT_EQ(blitzar_core_load_state(core, path.string().c_str(), "vtk", 5000u), BLITZAR_CORE_OK);
    expectFixtureParticles(readSnapshot(core));
}

/// Description: Executes the TEST operation.
TEST(BlitzarCoreApiTest, TST_UNT_CORE_004_LoadsAsciiVtkGoldenFile)
{
    blitzar_core_t* core = createCore(makeCpuConfig());
    ASSERT_NE(core, nullptr);
    loadFixtureAndExpect(core, fixturePath("vtk_fixture_ascii.vtk"));
    blitzar_core_destroy(core);
}

/// Description: Executes the TEST operation.
TEST(BlitzarCoreApiTest, TST_UNT_CORE_005_LoadsBinaryVtkGoldenFile)
{
    blitzar_core_t* core = createCore(makeCpuConfig());
    ASSERT_NE(core, nullptr);
    loadFixtureAndExpect(core, fixturePath("vtk_fixture_binary.vtk"));
    blitzar_core_destroy(core);
}

/// Description: Executes the TEST operation.
TEST(BlitzarCoreApiTest, TST_UNT_CORE_006_RoundTripsAsciiAndBinaryVtkWithinTolerance)
{
    const auto runRoundTrip = [](const char* fixtureName, const char* exportFormat,
                                 const char* encodingMarker) {
        blitzar_core_t* source = createCore(makeCpuConfig());
        ASSERT_NE(source, nullptr);
        loadFixtureAndExpect(source, fixturePath(fixtureName));
        const std::filesystem::path exportPath = makeTempPath("blitzar_vtk_roundtrip", ".vtk");
        ASSERT_EQ(
            blitzar_core_export_state(source, exportPath.string().c_str(), exportFormat, 5000u),
            BLITZAR_CORE_OK);
        expectFileContains(exportPath, encodingMarker);
        blitzar_core_destroy(source);
        blitzar_core_t* target = createCore(makeCpuConfig());
        ASSERT_NE(target, nullptr);
        loadFixtureAndExpect(target, exportPath);
        blitzar_core_destroy(target);
        std::filesystem::remove(exportPath);
    };
    runRoundTrip("vtk_fixture_ascii.vtk", "vtk", "ASCII");
    runRoundTrip("vtk_fixture_binary.vtk", "vtk_binary", "BINARY");
}

/// Description: Executes the TEST operation.
TEST(BlitzarCoreApiTest, TST_UNT_CORE_007_InvalidAndCorruptVtkInputsFallBackDeterministically)
{
    const auto expectGeneratedFallback = [](const std::filesystem::path& path, const char* format) {
        const blitzar_core_config_t config = makeCpuConfig();
        blitzar_core_t* core = createCore(config);
        ASSERT_NE(core, nullptr);
        ASSERT_EQ(blitzar_core_load_state(core, path.string().c_str(), format, 5000u),
                  BLITZAR_CORE_OK);
        const std::vector<blitzar_render_particle_t> snapshot = readSnapshot(core);
        EXPECT_EQ(snapshot.size(), config.particle_count);
        blitzar_core_status_t status{};
        ASSERT_EQ(blitzar_core_get_status(core, &status), BLITZAR_CORE_OK);
        EXPECT_EQ(status.faulted, 0u);
        blitzar_core_destroy(core);
    };
    const std::filesystem::path missingPath = makeTempPath("blitzar_missing_fixture", ".vtk");
    expectGeneratedFallback(missingPath, "vtk");
    const std::filesystem::path corruptAsciiPath = makeTempPath("blitzar_corrupt_ascii", ".vtk");
    {
        std::ifstream in(fixturePath("vtk_fixture_ascii.vtk"));
        ASSERT_TRUE(in.is_open());
        std::ofstream out(corruptAsciiPath, std::ios::trunc);
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
    expectGeneratedFallback(corruptAsciiPath, "vtk");
    std::filesystem::remove(corruptAsciiPath);
    const std::filesystem::path corruptBinaryPath = makeTempPath("blitzar_corrupt_binary", ".vtk");
    {
        std::ifstream in(fixturePath("vtk_fixture_binary.vtk"), std::ios::binary);
        ASSERT_TRUE(in.is_open());
        const std::string content((std::istreambuf_iterator<char>(in)),
                                  std::istreambuf_iterator<char>());
        ASSERT_GT(content.size(), 32u);
        std::ofstream out(corruptBinaryPath, std::ios::binary | std::ios::trunc);
        ASSERT_TRUE(out.is_open());
        out.write(content.data(), static_cast<std::streamsize>(content.size() - 8u));
    }
    expectGeneratedFallback(corruptBinaryPath, "vtk");
    std::filesystem::remove(corruptBinaryPath);
}
