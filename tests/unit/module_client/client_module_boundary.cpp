#include "client/ClientModuleBoundary.hpp"
#include "client/ClientModuleHash.hpp"
#include "client/ClientModuleManifest.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

namespace grav_test_client_host_boundary {

TEST(ClientModuleBoundaryTest, TST_UNT_MODHOST_004_CreateResultTracksOpaqueModuleState)
{
    int payload = 42;
    grav_module::ClientModuleCreateResult createResult;

    *createResult.rawSlot() = &payload;

    ASSERT_TRUE(createResult.hasValue());
    const grav_module::ClientModuleOpaqueState moduleState = createResult.state();
    EXPECT_TRUE(moduleState.hasValue());
    EXPECT_EQ(moduleState.rawPointer(), &payload);
}

TEST(ClientModuleBoundaryTest, TST_UNT_MODHOST_005_ErrorBufferViewWritesTruncatedMessage)
{
    char buffer[6] = {};
    const grav_client::ErrorBufferView errorBuffer(buffer, sizeof(buffer));

    errorBuffer.write("abcdefghi");

    EXPECT_EQ(std::string(buffer), "abcde");
}

TEST(ClientModuleBoundaryTest, TST_UNT_MODHOST_006_CommandControlUpdatesKeepRunningFlag)
{
    grav_module::ClientModuleCommandResult commandResult;
    const grav_module::ClientModuleCommandControl commandControl(commandResult.rawKeepRunningFlag());

    EXPECT_TRUE(commandResult.keepRunning());
    commandControl.requestStop();
    EXPECT_FALSE(commandResult.keepRunning());
    commandControl.setContinue();
    EXPECT_TRUE(commandResult.keepRunning());
}

TEST(ClientModuleBoundaryTest, TST_UNT_MODHOST_009_ModuleManifestAndHashValidateExpectedModule)
{
    const std::filesystem::path tempRoot = std::filesystem::temp_directory_path() / "gravity-module-manifest-ok";
    std::filesystem::create_directories(tempRoot);
    const std::filesystem::path modulePath = tempRoot / "gravityClientModuleCli.dll";
    {
        std::ofstream output(modulePath, std::ios::binary);
        output << "abc";
    }
    std::ofstream manifest(modulePath.string() + ".manifest", std::ios::binary);
    manifest
        << "format_version=1\n"
        << "module_id=cli\n"
        << "module_name=gravityClientModuleCli\n"
        << "api_version=1\n"
        << "product_name=A.S.T.E.R.\n"
        << "product_version=0.0.0-dev\n"
        << "library_file=gravityClientModuleCli.dll\n"
        << "sha256=ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad\n";
    manifest.close();

    grav_module::ClientModuleManifest parsed;
    std::string error;
    ASSERT_TRUE(grav_module::ClientModuleManifest::load(modulePath.string(), parsed, error));
    ASSERT_TRUE(parsed.validateForLoad(modulePath.string(), "cli", error));

    std::string digest;
    ASSERT_TRUE(grav_module::ClientModuleHash::computeFileSha256Hex(modulePath.string(), digest, error));
    EXPECT_EQ(digest, parsed.sha256());
}

TEST(ClientModuleBoundaryTest, TST_UNT_MODHOST_010_ModuleManifestRejectsUnsupportedModuleAndDigestMismatch)
{
    const std::filesystem::path tempRoot = std::filesystem::temp_directory_path() / "gravity-module-manifest-bad";
    std::filesystem::create_directories(tempRoot);
    const std::filesystem::path modulePath = tempRoot / "foreign.dll";
    {
        std::ofstream output(modulePath, std::ios::binary);
        output << "abcd";
    }
    std::ofstream manifest(modulePath.string() + ".manifest", std::ios::binary);
    manifest
        << "format_version=1\n"
        << "module_id=foreign\n"
        << "module_name=foreign-module\n"
        << "api_version=1\n"
        << "product_name=A.S.T.E.R.\n"
        << "product_version=0.0.0-dev\n"
        << "library_file=foreign.dll\n"
        << "sha256=ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad\n";
    manifest.close();

    grav_module::ClientModuleManifest parsed;
    std::string error;
    ASSERT_TRUE(grav_module::ClientModuleManifest::load(modulePath.string(), parsed, error));
    EXPECT_FALSE(parsed.validateForLoad(modulePath.string(), "", error));
    EXPECT_NE(error.find("unsupported module id"), std::string::npos);
}

} // namespace grav_test_client_host_boundary
