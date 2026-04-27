// File: tests/unit/module_cli/command_executor.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "command/CommandContext.hpp"
#include "command/CommandExecutor.hpp"
#include "command/CommandParser.hpp"
#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
namespace grav_test_module_cli_command_executor {
/// Description: Defines the FakeCommandTransport data or behavior contract.
class FakeCommandTransport final : public grav_cmd::CommandTransport {
public:
    /// Description: Executes the connect operation.
    bool connect(const std::string& host, std::uint16_t port) override
    {
        connected = true;
        connectHistory.emplace_back(host, port);
        return connectResult;
    }
    /// Description: Executes the disconnect operation.
    void disconnect() override
    {
        connected = false;
        disconnected = true;
    }
    /// Description: Executes the isConnected operation.
    bool isConnected() const override
    {
        return connected;
    }
    ServerClientResponse sendCommand(const std::string& cmd,
                                     const std::string& fieldsJson = "") override
    {
        commandHistory.emplace_back(cmd, fieldsJson);
        return nextCommandResponse;
    }
    /// Description: Executes the getStatus operation.
    ServerClientResponse getStatus(ServerClientStatus& outStatus) override
    {
        statusCallCount += 1u;
        if (statusCallCount <= scriptedStatuses.size()) {
            outStatus = scriptedStatuses[statusCallCount - 1u];
        }
        return nextStatusResponse;
    }
    bool connectResult = true;
    bool connected = false;
    bool disconnected = false;
    std::size_t statusCallCount = 0u;
    ServerClientResponse nextCommandResponse{true, {}, {}};
    ServerClientResponse nextStatusResponse{true, {}, {}};
    std::vector<std::pair<std::string, std::uint16_t>> connectHistory;
    std::vector<std::pair<std::string, std::string>> commandHistory;
    std::vector<ServerClientStatus> scriptedStatuses;
};
/// Description: Executes the parseSingle operation.
static grav_cmd::CommandRequest parseSingle(const std::string& line)
{
    const grav_cmd::CommandParseResult parsed = grav_cmd::CommandParser::parseLine(line, 1u);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(parsed.ok);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(parsed.requests.size(), 1u);
    return parsed.requests.front();
}
/// Description: Executes the TEST operation.
TEST(CommandExecutorTest, TST_UNT_MODCLI_009_SetDtMapsToProtocolCommand)
{
    FakeCommandTransport transport;
    grav_cmd::CommandSessionState session;
    std::ostringstream output;
    grav_cmd::CommandExecutionContext context{transport, session,
                                              grav_cmd::CommandExecutionMode::Interactive, output};
    const grav_cmd::CommandResult result =
        /// Description: Executes the execute operation.
        grav_cmd::CommandExecutor::execute(parseSingle("set_dt 0.25"), context);
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(result.ok);
    /// Description: Executes the ASSERT_EQ operation.
    ASSERT_EQ(transport.commandHistory.size(), 1u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(transport.commandHistory[0].first, "set_dt");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(transport.commandHistory[0].second, "\"value\":0.250000");
    /// Description: Executes the EXPECT_FLOAT_EQ operation.
    EXPECT_FLOAT_EQ(session.config.dt, 0.25F);
}
/// Description: Executes the TEST operation.
TEST(CommandExecutorTest, TST_UNT_MODCLI_010_RunStepsAndRunUntilUseDeterministicStepFlow)
{
    FakeCommandTransport transport;
    transport.connected = true;
    ServerClientStatus initialStatus{};
    initialStatus.steps = 0u;
    initialStatus.totalTime = 0.0F;
    ServerClientStatus middleStatus{};
    middleStatus.steps = 1u;
    middleStatus.totalTime = 0.5F;
    ServerClientStatus finalStatus{};
    finalStatus.steps = 2u;
    finalStatus.totalTime = 1.0F;
    transport.scriptedStatuses = {initialStatus, middleStatus, finalStatus};
    grav_cmd::CommandSessionState session;
    std::ostringstream output;
    grav_cmd::CommandExecutionContext context{transport, session,
                                              grav_cmd::CommandExecutionMode::Batch, output};
    const grav_cmd::CommandResult runSteps =
        /// Description: Executes the execute operation.
        grav_cmd::CommandExecutor::execute(parseSingle("run_steps 3"), context);
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(runSteps.ok);
    /// Description: Executes the ASSERT_EQ operation.
    ASSERT_EQ(transport.commandHistory.size(), 2u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(transport.commandHistory[0].first, "pause");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(transport.commandHistory[1].first, "step");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(transport.commandHistory[1].second, "\"count\":3");
    transport.commandHistory.clear();
    const grav_cmd::CommandResult runUntil =
        /// Description: Executes the execute operation.
        grav_cmd::CommandExecutor::execute(parseSingle("run_until 1.0"), context);
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(runUntil.ok);
    /// Description: Executes the ASSERT_EQ operation.
    ASSERT_EQ(transport.commandHistory.size(), 3u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(transport.commandHistory[0].first, "pause");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(transport.commandHistory[1].first, "step");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(transport.commandHistory[2].first, "step");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(transport.statusCallCount, 3u);
}
/// Description: Executes the TEST operation.
TEST(CommandExecutorTest, TST_UNT_MODCLI_011_LoadConfigUpdatesSessionPathAndRejectsInvalidProfile)
{
    FakeCommandTransport transport;
    grav_cmd::CommandSessionState session;
    session.configPath = "before.ini";
    std::ostringstream output;
    grav_cmd::CommandExecutionContext context{transport, session,
                                              grav_cmd::CommandExecutionMode::Batch, output};
    const grav_cmd::CommandResult invalidProfile =
        /// Description: Executes the execute operation.
        grav_cmd::CommandExecutor::execute(parseSingle("set_profile impossible"), context);
    /// Description: Executes the ASSERT_FALSE operation.
    ASSERT_FALSE(invalidProfile.ok);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(invalidProfile.message, "invalid performance profile");
    const std::string originalPath = session.configPath;
    const grav_cmd::CommandResult loadResult =
        /// Description: Executes the execute operation.
        grav_cmd::CommandExecutor::execute(parseSingle("load_config simulation.ini"), context);
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(loadResult.ok);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(session.configPath, "simulation.ini");
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(session.configPath, originalPath);
}
/// Description: Executes the TEST operation.
TEST(CommandExecutorTest, TST_UNT_MODCLI_012_CheckpointCommandsMapToProtocolCommands)
{
    FakeCommandTransport transport;
    grav_cmd::CommandSessionState session;
    std::ostringstream output;
    grav_cmd::CommandExecutionContext context{transport, session,
                                              grav_cmd::CommandExecutionMode::Batch, output};
    const grav_cmd::CommandResult saveResult = grav_cmd::CommandExecutor::execute(
        /// Description: Executes the parseSingle operation.
        parseSingle("save_checkpoint checkpoints/demo.chk"), context);
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(saveResult.ok);
    /// Description: Executes the ASSERT_EQ operation.
    ASSERT_EQ(transport.commandHistory.size(), 1u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(transport.commandHistory[0].first, "save_checkpoint");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(transport.commandHistory[0].second, "\"path\":\"checkpoints/demo.chk\"");
    transport.commandHistory.clear();
    const grav_cmd::CommandResult loadResult = grav_cmd::CommandExecutor::execute(
        /// Description: Executes the parseSingle operation.
        parseSingle("load_checkpoint checkpoints/demo.chk"), context);
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(loadResult.ok);
    /// Description: Executes the ASSERT_EQ operation.
    ASSERT_EQ(transport.commandHistory.size(), 1u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(transport.commandHistory[0].first, "load_checkpoint");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(transport.commandHistory[0].second, "\"path\":\"checkpoints/demo.chk\"");
}
} // namespace grav_test_module_cli_command_executor
