#include "client/ClientCommon.hpp"

#include "config/SimulationConfig.hpp"
#include "protocol/ServerProtocol.hpp"

#include <gtest/gtest.h>

#include <regex>
#include <string>

namespace grav_test_client_common {

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
    EXPECT_NE(withDirectory.find("out"), std::string::npos);
    EXPECT_TRUE(std::regex_search(withDirectory, std::regex("sim_[0-9]{8}_[0-9]{6}_s42\\.vtk$")));

    const std::string defaultPath = grav_client::buildSuggestedExportPath("", "", 9u);
    EXPECT_NE(defaultPath.find("exports"), std::string::npos);
    EXPECT_TRUE(std::regex_search(defaultPath, std::regex("sim_[0-9]{8}_[0-9]{6}_s9\\.vtk$")));
}

} // namespace grav_test_client_common
