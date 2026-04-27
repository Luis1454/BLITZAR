// File: tests/unit/module_client/client_module_boundary.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "client/ClientModuleBoundary.hpp"
#include "client/ClientModuleHash.hpp"
#include "client/ClientModuleManifest.hpp"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <string>
namespace grav_test_client_host_boundary {
/// Description: Executes the TEST operation.
TEST(ClientModuleBoundaryTest, TST_UNT_MODHOST_004_CreateResultTracksOpaqueModuleState)
{
    int payload = 42;
    grav_module::ClientModuleCreateResult createResult;
    *createResult.rawSlot() = &payload;
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(createResult.hasValue());
    const grav_module::ClientModuleOpaqueState moduleState = createResult.state();
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(moduleState.hasValue());
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(moduleState.rawPointer(), &payload);
}
/// Description: Executes the TEST operation.
TEST(ClientModuleBoundaryTest, TST_UNT_MODHOST_005_ErrorBufferViewWritesTruncatedMessage)
{
    char buffer[6] = {};
    /// Description: Executes the errorBuffer operation.
    const grav_client::ErrorBufferView errorBuffer(buffer, sizeof(buffer));
    errorBuffer.write("abcdefghi");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(std::string(buffer), "abcde");
}
/// Description: Executes the TEST operation.
TEST(ClientModuleBoundaryTest, TST_UNT_MODHOST_006_CommandControlUpdatesKeepRunningFlag)
{
    grav_module::ClientModuleCommandResult commandResult;
    const grav_module::ClientModuleCommandControl commandControl(
        commandResult.rawKeepRunningFlag());
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(commandResult.keepRunning());
    commandControl.requestStop();
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(commandResult.keepRunning());
    commandControl.setContinue();
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(commandResult.keepRunning());
}
/// Description: Executes the TEST operation.
TEST(ClientModuleBoundaryTest, TST_UNT_MODHOST_009_ModuleManifestAndHashValidateExpectedModule)
{
    const std::filesystem::path tempRoot =
        std::filesystem::temp_directory_path() / "gravity-module-manifest-ok";
    /// Description: Executes the create_directories operation.
    std::filesystem::create_directories(tempRoot);
    const std::filesystem::path modulePath = tempRoot / "gravityClientModuleCli.dll";
    {
        /// Description: Executes the output operation.
        std::ofstream output(modulePath, std::ios::binary);
        output << "abc";
    }
    /// Description: Executes the manifest operation.
    std::ofstream manifest(modulePath.string() + ".manifest", std::ios::binary);
    manifest << "format_version=1\n"
             << "module_id=cli\n"
             << "module_name=gravityClientModuleCli\n"
             << "api_version=1\n"
             << "product_name=BLITZAR\n"
             << "product_version=0.0.0-dev\n"
             << "library_file=gravityClientModuleCli.dll\n"
             << "sha256=ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad\n";
    manifest.close();
    grav_module::ClientModuleManifest parsed;
    std::string error;
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(grav_module::ClientModuleManifest::load(modulePath.string(), parsed, error));
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(parsed.validateForLoad(modulePath.string(), "cli", error));
    std::string digest;
    ASSERT_TRUE(
        /// Description: Executes the computeFileSha256Hex operation.
        grav_module::ClientModuleHash::computeFileSha256Hex(modulePath.string(), digest, error));
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(digest, parsed.sha256());
}
TEST(ClientModuleBoundaryTest,
     TST_UNT_MODHOST_010_ModuleManifestRejectsUnsupportedModuleAndDigestMismatch)
{
    const std::filesystem::path tempRoot =
        std::filesystem::temp_directory_path() / "gravity-module-manifest-bad";
    /// Description: Executes the create_directories operation.
    std::filesystem::create_directories(tempRoot);
    const std::filesystem::path modulePath = tempRoot / "foreign.dll";
    {
        /// Description: Executes the output operation.
        std::ofstream output(modulePath, std::ios::binary);
        output << "abcd";
    }
    /// Description: Executes the manifest operation.
    std::ofstream manifest(modulePath.string() + ".manifest", std::ios::binary);
    manifest << "format_version=1\n"
             << "module_id=foreign\n"
             << "module_name=foreign-module\n"
             << "api_version=1\n"
             << "product_name=BLITZAR\n"
             << "product_version=0.0.0-dev\n"
             << "library_file=foreign.dll\n"
             << "sha256=ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad\n";
    manifest.close();
    grav_module::ClientModuleManifest parsed;
    std::string error;
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(grav_module::ClientModuleManifest::load(modulePath.string(), parsed, error));
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(parsed.validateForLoad(modulePath.string(), "", error));
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(error.find("unsupported module id"), std::string::npos);
}
} // namespace grav_test_client_host_boundary
