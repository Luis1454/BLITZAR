#include "client/ClientModuleBoundary.hpp"

#include <gtest/gtest.h>

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

} // namespace grav_test_client_host_boundary
