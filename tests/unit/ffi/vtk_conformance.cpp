/*
 * @file tests/unit/ffi/vtk_conformance.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Automated verification assets for BLITZAR quality gates.
 */

#include "ffi/BlitzarCoreApi.hpp"
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <string>
#include <vector>

/*
 * @brief Defines the expected core particle type contract.
 * @param None This contract does not take explicit parameters.
 * @return Not applicable; this block documents a type contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
struct ExpectedCoreParticle {
    float x;
    float y;
    float z;
    float mass;
    float temperature;
};

/*
 * @brief Documents the make cpu config operation contract.
 * @param None This contract does not take explicit parameters.
 * @return blitzar_core_config_t value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
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

/*
 * @brief Documents the create core operation contract.
 * @param config Input value used by this contract.
 * @return blitzar_core_t* value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
static blitzar_core_t* createCore(const blitzar_core_config_t& config)
{
    char error[BLITZAR_CORE_ERROR_CAPACITY] = {};
    blitzar_core_t* core = blitzar_core_create(&config, error, sizeof(error));
    EXPECT_NE(core, nullptr) << error;
    return core;
}

/*
 * @brief Documents the fixture path operation contract.
 * @param fileName Input value used by this contract.
 * @return std::filesystem::path value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
static std::filesystem::path fixturePath(const char* fileName)
{
    const std::filesystem::path sourceFile(__FILE__);
    return sourceFile.parent_path().parent_path().parent_path() / "data" / fileName;
}

/*
 * @brief Documents the make temp path operation contract.
 * @param stem Input value used by this contract.
 * @param extension Input value used by this contract.
 * @return std::filesystem::path value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
static std::filesystem::path makeTempPath(const char* stem, const char* extension)
{
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return std::filesystem::temp_directory_path() /
           (std::string(stem) + "_" + std::to_string(stamp) + extension);
}

/*
 * @brief Documents the read snapshot operation contract.
 * @param core Input value used by this contract.
 * @return std::vector<blitzar_render_particle_t> value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
static std::vector<blitzar_render_particle_t> readSnapshot(const blitzar_core_t* core)
{
    blitzar_core_snapshot_t snapshot{};
    EXPECT_EQ(blitzar_core_get_snapshot(core, 0u, &snapshot), BLITZAR_CORE_OK);
    std::vector<blitzar_render_particle_t> particles(snapshot.particles,
                                                     snapshot.particles + snapshot.count);
    blitzar_core_free_snapshot(&snapshot);
    return particles;
}

/*
 * @brief Documents the expect particle near operation contract.
 * @param actual Input value used by this contract.
 * @param expected Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
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

/*
 * @brief Documents the expect fixture particles operation contract.
 * @param particles Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
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

/*
 * @brief Documents the expect file contains operation contract.
 * @param path Input value used by this contract.
 * @param needle Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
static void expectFileContains(const std::filesystem::path& path, const std::string& needle)
{
    std::ifstream in(path, std::ios::binary);
    ASSERT_TRUE(in.is_open()) << path.string();
    const std::string content((std::istreambuf_iterator<char>(in)),
                              std::istreambuf_iterator<char>());
    EXPECT_NE(content.find(needle), std::string::npos) << path.string();
}

/*
 * @brief Documents the load fixture and expect operation contract.
 * @param core Input value used by this contract.
 * @param path Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
static void loadFixtureAndExpect(blitzar_core_t* core, const std::filesystem::path& path)
{
    ASSERT_EQ(blitzar_core_load_state(core, path.string().c_str(), "vtk", 5000u), BLITZAR_CORE_OK);
    expectFixtureParticles(readSnapshot(core));
}

TEST(BlitzarCoreApiTest, TST_UNT_CORE_004_LoadsAsciiVtkGoldenFile)
{
    blitzar_core_t* core = createCore(makeCpuConfig());
    ASSERT_NE(core, nullptr);
    loadFixtureAndExpect(core, fixturePath("vtk_fixture_ascii.vtk"));
    blitzar_core_destroy(core);
}

TEST(BlitzarCoreApiTest, TST_UNT_CORE_005_LoadsBinaryVtkGoldenFile)
{
    blitzar_core_t* core = createCore(makeCpuConfig());
    ASSERT_NE(core, nullptr);
    loadFixtureAndExpect(core, fixturePath("vtk_fixture_binary.vtk"));
    blitzar_core_destroy(core);
}

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
