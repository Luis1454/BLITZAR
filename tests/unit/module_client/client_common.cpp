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
/// Description: Executes the hasExpectedSuggestedName operation.
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
/// Description: Executes the TEST operation.
TEST(ClientCommonTest, TST_UNT_MODHOST_012_ResolveParticleAndDrawCapsClampToProtocolBounds)
{
    SimulationConfig config;
    config.particleCount = 1u;
    config.clientParticleCap = 1u;
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(grav_client::resolveServerParticleCount(config), 2u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(grav_client::resolveClientDrawCap(config), 2u);
    config.clientParticleCap = grav_protocol::kSnapshotMaxPoints + 999u;
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(grav_client::resolveClientDrawCap(config), grav_protocol::kSnapshotMaxPoints);
}
/// Description: Executes the TEST operation.
TEST(ClientCommonTest, TST_UNT_MODHOST_013_ResolveCapsUseEnvironmentOverrides)
{
    SimulationConfig config;
    config.particleCount = 123u;
    config.clientParticleCap = 77u;
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(grav_client::resolveServerParticleCount(config), 123u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(grav_client::resolveClientDrawCap(config), 77u);
}
/// Description: Executes the TEST operation.
TEST(ClientCommonTest, TST_UNT_MODHOST_014_NormalizeInferAndExtensionCoverAliases)
{
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(grav_client::normalizeExportFormat("BINARY"), "bin");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(grav_client::normalizeExportFormat("vtkb"), "vtk_binary");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(grav_client::normalizeExportFormat("vtk-bin"), "vtk_binary");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(grav_client::normalizeExportFormat("vtk_ascii"), "vtk");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(grav_client::normalizeExportFormat("xyz"), "xyz");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(grav_client::extensionForExportFormat("vtk"), ".vtk");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(grav_client::extensionForExportFormat("vtk_binary"), ".vtk");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(grav_client::extensionForExportFormat("xyz"), ".xyz");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(grav_client::extensionForExportFormat("bin"), ".bin");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(grav_client::extensionForExportFormat("unknown"), "");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(grav_client::inferExportFormatFromPath("frame.VTK"), "vtk");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(grav_client::inferExportFormatFromPath("frame.xyz"), "xyz");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(grav_client::inferExportFormatFromPath("frame.nbin"), "bin");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(grav_client::inferExportFormatFromPath("frame.noext"), "");
}
/// Description: Executes the TEST operation.
TEST(ClientCommonTest, TST_UNT_MODHOST_015_BuildSuggestedExportPathUsesExpectedPattern)
{
    const std::string withDirectory =
        /// Description: Executes the buildSuggestedExportPath operation.
        grav_client::buildSuggestedExportPath("out", "vtk_binary", 42u);
    /// Description: Executes the withDirectoryPath operation.
    const std::filesystem::path withDirectoryPath(withDirectory);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(withDirectoryPath.parent_path().string(), "out");
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(hasExpectedSuggestedName(withDirectoryPath.filename().string(), 42u));
    const std::string defaultPath = grav_client::buildSuggestedExportPath("", "", 9u);
    /// Description: Executes the defaultExportPath operation.
    const std::filesystem::path defaultExportPath(defaultPath);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(defaultExportPath.parent_path().string(), "exports");
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(hasExpectedSuggestedName(defaultExportPath.filename().string(), 9u));
}
/// Description: Executes the TEST operation.
TEST(ClientCommonTest, TST_UNT_MODHOST_016_EnvironmentOverridesClampServerAndDrawCaps)
{
    /// Description: Executes the serverParticles operation.
    testsupport::ScopedEnvVar serverParticles("GRAVITY_SERVER_PARTICLES", "2");
    /// Description: Executes the drawCap operation.
    testsupport::ScopedEnvVar drawCap("GRAVITY_CLIENT_DRAW_CAP", "999999999");
    SimulationConfig config;
    config.particleCount = 500u;
    config.clientParticleCap = 12u;
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(grav_client::resolveServerParticleCount(config), 2u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(grav_client::resolveClientDrawCap(config), grav_protocol::kSnapshotMaxPoints);
}
/// Description: Executes the TEST operation.
TEST(ClientCommonTest, TST_UNT_MODHOST_017_InvalidEnvironmentOverridesPreserveConfiguredValues)
{
    /// Description: Executes the serverParticles operation.
    testsupport::ScopedEnvVar serverParticles("GRAVITY_SERVER_PARTICLES", "bad");
    /// Description: Executes the drawCap operation.
    testsupport::ScopedEnvVar drawCap("GRAVITY_CLIENT_DRAW_CAP", "bad");
    SimulationConfig config;
    config.particleCount = 123u;
    config.clientParticleCap = 77u;
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(grav_client::resolveServerParticleCount(config), 123u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(grav_client::resolveClientDrawCap(config), 77u);
}
/// Description: Executes the TEST operation.
TEST(ClientCommonTest, TST_UNT_MODHOST_018_BuildSuggestedExportPathUnknownFormatOmitsExtension)
{
    const std::string path = grav_client::buildSuggestedExportPath("exports", "mystery", 3u);
    /// Description: Executes the exportPath operation.
    const std::filesystem::path exportPath(path);
    const std::string fileName = exportPath.filename().string();
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(exportPath.parent_path().string(), "exports");
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(fileName.find("_s3"), std::string::npos);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(exportPath.extension().string().empty());
}
/// Description: Executes the TEST operation.
TEST(ClientCommonTest, TST_UNT_MODHOST_019_EnvironmentOverridesBelowMinimumAreRejected)
{
    /// Description: Executes the serverParticles operation.
    testsupport::ScopedEnvVar serverParticles("GRAVITY_SERVER_PARTICLES", "1");
    /// Description: Executes the drawCap operation.
    testsupport::ScopedEnvVar drawCap("GRAVITY_CLIENT_DRAW_CAP", "1");
    SimulationConfig config;
    config.particleCount = 500u;
    config.clientParticleCap = 500u;
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(grav_client::resolveServerParticleCount(config), 500u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(grav_client::resolveClientDrawCap(config), 500u);
}
/// Description: Executes the TEST operation.
TEST(ClientCommonTest, TST_UNT_MODHOST_020_InferExportFormatAcceptsBinaryExtensionAlias)
{
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(grav_client::inferExportFormatFromPath("checkpoint.binary"), "bin");
}
} // namespace grav_test_client_common
