#include "client/ClientCommon.hpp"

#include "config/SimulationConfig.hpp"
#include "protocol/ServerProtocol.hpp"

#include <gtest/gtest.h>

#include <cctype>
#include <filesystem>
#include <string>

namespace grav_test_client_common {

static bool hasExpectedSuggestedName(const std::string &fileName, std::uint64_t step)
{
    if (fileName.size() < 26u || fileName.rfind("sim_", 0u) != 0u) {
        return false;
    }
    for (std::size_t i = 4u; i < 12u; ++i) {
        if (!std::isdigit(static_cast<unsigned char>(fileName[i]))) {
            return false;
        }
    }
    if (fileName[12] != '_') {
        return false;
    }
    for (std::size_t i = 13u; i < 19u; ++i) {
        if (!std::isdigit(static_cast<unsigned char>(fileName[i]))) {
            return false;
        }
    }
    const std::string expectedSuffix = "_s" + std::to_string(step) + ".vtk";
    return fileName.compare(fileName.size() - expectedSuffix.size(), expectedSuffix.size(), expectedSuffix) == 0;
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
    const std::string withDirectory = grav_client::buildSuggestedExportPath("out", "vtk_binary", 42u);
    const std::filesystem::path withDirectoryPath(withDirectory);
    EXPECT_EQ(withDirectoryPath.parent_path().string(), "out");
    EXPECT_TRUE(hasExpectedSuggestedName(withDirectoryPath.filename().string(), 42u));

    const std::string defaultPath = grav_client::buildSuggestedExportPath("", "", 9u);
    const std::filesystem::path defaultExportPath(defaultPath);
    EXPECT_EQ(defaultExportPath.parent_path().string(), "exports");
    EXPECT_TRUE(hasExpectedSuggestedName(defaultExportPath.filename().string(), 9u));
}

} // namespace grav_test_client_common
