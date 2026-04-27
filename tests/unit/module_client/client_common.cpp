// File: tests/unit/module_client/client_common.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "client/ClientCommon.hpp"
#include "config/SimulationConfig.hpp"
#include "protocol/ServerProtocol.hpp"
#include "tests/support/scoped_env_var.hpp"
#include <cctype>
#include <filesystem>
#include <gtest/gtest.h>
#include <string>
namespace grav_test_client_common {
static bool hasExpectedSuggestedName(const std::string& fileName, std::uint64_t step)
{
    if (fileName.size() < 26u || fileName.rfind("sim_", 0u) != 0u) {
        return false;
    }
    for (std::size_t i = 4u; i < 12u; ++i)
        if (!std::isdigit(static_cast<unsigned char>(fileName[i]))) {
            return false;
        }
    if (fileName[12] != '_')
        return false;
    for (std::size_t i = 13u; i < 19u; ++i)
        if (!std::isdigit(static_cast<unsigned char>(fileName[i]))) {
            return false;
        }
    const std::string expectedSuffix = "_s" + std::to_string(step) + ".vtk";
    return fileName.compare(fileName.size() - expectedSuffix.size(), expectedSuffix.size(),
                            expectedSuffix) == 0;
}
TEST(ClientCommonTest, TST_UNT_MODHOST_012_ResolveParticleAndDrawCapsClampToProtocolBounds)
{
    SimulationConfig config;
    config.particleCount = 1u;
    config.clientParticleCap = 1u;
    EXPECT_EQ(grav_client::resolveServerParticleCount(config), 2u);
    EXPECT_EQ(grav_client::resolveClientDrawCap(config), 2u);
    config.clientParticleCap = grav_protocol::kSnapshotMaxPoints + 999u;
    EXPECT_EQ(grav_client::resolveClientDrawCap(config), grav_protocol::kSnapshotMaxPoints);
}
TEST(ClientCommonTest, TST_UNT_MODHOST_013_ResolveCapsUseEnvironmentOverrides)
{
    SimulationConfig config;
    config.particleCount = 123u;
    config.clientParticleCap = 77u;
    EXPECT_EQ(grav_client::resolveServerParticleCount(config), 123u);
    EXPECT_EQ(grav_client::resolveClientDrawCap(config), 77u);
}
TEST(ClientCommonTest, TST_UNT_MODHOST_014_NormalizeInferAndExtensionCoverAliases)
{
    EXPECT_EQ(grav_client::normalizeExportFormat("BINARY"), "bin");
    EXPECT_EQ(grav_client::normalizeExportFormat("vtkb"), "vtk_binary");
    EXPECT_EQ(grav_client::normalizeExportFormat("vtk-bin"), "vtk_binary");
    EXPECT_EQ(grav_client::normalizeExportFormat("vtk_ascii"), "vtk");
    EXPECT_EQ(grav_client::normalizeExportFormat("xyz"), "xyz");
    EXPECT_EQ(grav_client::extensionForExportFormat("vtk"), ".vtk");
    EXPECT_EQ(grav_client::extensionForExportFormat("vtk_binary"), ".vtk");
    EXPECT_EQ(grav_client::extensionForExportFormat("xyz"), ".xyz");
    EXPECT_EQ(grav_client::extensionForExportFormat("bin"), ".bin");
    EXPECT_EQ(grav_client::extensionForExportFormat("unknown"), "");
    EXPECT_EQ(grav_client::inferExportFormatFromPath("frame.VTK"), "vtk");
    EXPECT_EQ(grav_client::inferExportFormatFromPath("frame.xyz"), "xyz");
    EXPECT_EQ(grav_client::inferExportFormatFromPath("frame.nbin"), "bin");
    EXPECT_EQ(grav_client::inferExportFormatFromPath("frame.noext"), "");
}
TEST(ClientCommonTest, TST_UNT_MODHOST_015_BuildSuggestedExportPathUsesExpectedPattern)
{
    const std::string withDirectory =
        grav_client::buildSuggestedExportPath("out", "vtk_binary", 42u);
    const std::filesystem::path withDirectoryPath(withDirectory);
    EXPECT_EQ(withDirectoryPath.parent_path().string(), "out");
    EXPECT_TRUE(hasExpectedSuggestedName(withDirectoryPath.filename().string(), 42u));
    const std::string defaultPath = grav_client::buildSuggestedExportPath("", "", 9u);
    const std::filesystem::path defaultExportPath(defaultPath);
    EXPECT_EQ(defaultExportPath.parent_path().string(), "exports");
    EXPECT_TRUE(hasExpectedSuggestedName(defaultExportPath.filename().string(), 9u));
}
TEST(ClientCommonTest, TST_UNT_MODHOST_016_EnvironmentOverridesClampServerAndDrawCaps)
{
    testsupport::ScopedEnvVar serverParticles("GRAVITY_SERVER_PARTICLES", "2");
    testsupport::ScopedEnvVar drawCap("GRAVITY_CLIENT_DRAW_CAP", "999999999");
    SimulationConfig config;
    config.particleCount = 500u;
    config.clientParticleCap = 12u;
    EXPECT_EQ(grav_client::resolveServerParticleCount(config), 2u);
    EXPECT_EQ(grav_client::resolveClientDrawCap(config), grav_protocol::kSnapshotMaxPoints);
}
TEST(ClientCommonTest, TST_UNT_MODHOST_017_InvalidEnvironmentOverridesPreserveConfiguredValues)
{
    testsupport::ScopedEnvVar serverParticles("GRAVITY_SERVER_PARTICLES", "bad");
    testsupport::ScopedEnvVar drawCap("GRAVITY_CLIENT_DRAW_CAP", "bad");
    SimulationConfig config;
    config.particleCount = 123u;
    config.clientParticleCap = 77u;
    EXPECT_EQ(grav_client::resolveServerParticleCount(config), 123u);
    EXPECT_EQ(grav_client::resolveClientDrawCap(config), 77u);
}
TEST(ClientCommonTest, TST_UNT_MODHOST_018_BuildSuggestedExportPathUnknownFormatOmitsExtension)
{
    const std::string path = grav_client::buildSuggestedExportPath("exports", "mystery", 3u);
    const std::filesystem::path exportPath(path);
    const std::string fileName = exportPath.filename().string();
    EXPECT_EQ(exportPath.parent_path().string(), "exports");
    EXPECT_NE(fileName.find("_s3"), std::string::npos);
    EXPECT_TRUE(exportPath.extension().string().empty());
}
TEST(ClientCommonTest, TST_UNT_MODHOST_019_EnvironmentOverridesBelowMinimumAreRejected)
{
    testsupport::ScopedEnvVar serverParticles("GRAVITY_SERVER_PARTICLES", "1");
    testsupport::ScopedEnvVar drawCap("GRAVITY_CLIENT_DRAW_CAP", "1");
    SimulationConfig config;
    config.particleCount = 500u;
    config.clientParticleCap = 500u;
    EXPECT_EQ(grav_client::resolveServerParticleCount(config), 500u);
    EXPECT_EQ(grav_client::resolveClientDrawCap(config), 500u);
}
TEST(ClientCommonTest, TST_UNT_MODHOST_020_InferExportFormatAcceptsBinaryExtensionAlias)
{
    EXPECT_EQ(grav_client::inferExportFormatFromPath("checkpoint.binary"), "bin");
}
} // namespace grav_test_client_common
