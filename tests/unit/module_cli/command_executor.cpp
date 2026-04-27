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
class FakeCommandTransport final : public grav_cmd::CommandTransport {
public:
    bool connect(const std::string& host, std::uint16_t port) override
    {
        connected = true;
        connectHistory.emplace_back(host, port);
        return connectResult;
    }
    void disconnect() override
    {
        connected = false;
        disconnected = true;
    }
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
static grav_cmd::CommandRequest parseSingle(const std::string& line)
{
    const grav_cmd::CommandParseResult parsed = grav_cmd::CommandParser::parseLine(line, 1u);
    EXPECT_TRUE(parsed.ok);
    EXPECT_EQ(parsed.requests.size(), 1u);
    return parsed.requests.front();
}
TEST(CommandExecutorTest, TST_UNT_MODCLI_009_SetDtMapsToProtocolCommand)
{
    FakeCommandTransport transport;
    grav_cmd::CommandSessionState session;
    std::ostringstream output;
    grav_cmd::CommandExecutionContext context{transport, session,
                                              grav_cmd::CommandExecutionMode::Interactive, output};
    const grav_cmd::CommandResult result =
        grav_cmd::CommandExecutor::execute(parseSingle("set_dt 0.25"), context);
    ASSERT_TRUE(result.ok);
    ASSERT_EQ(transport.commandHistory.size(), 1u);
    EXPECT_EQ(transport.commandHistory[0].first, "set_dt");
    EXPECT_EQ(transport.commandHistory[0].second, "\"value\":0.250000");
    EXPECT_FLOAT_EQ(session.config.dt, 0.25F);
}
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
        grav_cmd::CommandExecutor::execute(parseSingle("run_steps 3"), context);
    ASSERT_TRUE(runSteps.ok);
    ASSERT_EQ(transport.commandHistory.size(), 2u);
    EXPECT_EQ(transport.commandHistory[0].first, "pause");
    EXPECT_EQ(transport.commandHistory[1].first, "step");
    EXPECT_EQ(transport.commandHistory[1].second, "\"count\":3");
    transport.commandHistory.clear();
    const grav_cmd::CommandResult runUntil =
        grav_cmd::CommandExecutor::execute(parseSingle("run_until 1.0"), context);
    ASSERT_TRUE(runUntil.ok);
    ASSERT_EQ(transport.commandHistory.size(), 3u);
    EXPECT_EQ(transport.commandHistory[0].first, "pause");
    EXPECT_EQ(transport.commandHistory[1].first, "step");
    EXPECT_EQ(transport.commandHistory[2].first, "step");
    EXPECT_EQ(transport.statusCallCount, 3u);
}
TEST(CommandExecutorTest, TST_UNT_MODCLI_011_LoadConfigUpdatesSessionPathAndRejectsInvalidProfile)
{
    FakeCommandTransport transport;
    grav_cmd::CommandSessionState session;
    session.configPath = "before.ini";
    std::ostringstream output;
    grav_cmd::CommandExecutionContext context{transport, session,
                                              grav_cmd::CommandExecutionMode::Batch, output};
    const grav_cmd::CommandResult invalidProfile =
        grav_cmd::CommandExecutor::execute(parseSingle("set_profile impossible"), context);
    ASSERT_FALSE(invalidProfile.ok);
    EXPECT_EQ(invalidProfile.message, "invalid performance profile");
    const std::string originalPath = session.configPath;
    const grav_cmd::CommandResult loadResult =
        grav_cmd::CommandExecutor::execute(parseSingle("load_config simulation.ini"), context);
    ASSERT_TRUE(loadResult.ok);
    EXPECT_EQ(session.configPath, "simulation.ini");
    EXPECT_NE(session.configPath, originalPath);
}
TEST(CommandExecutorTest, TST_UNT_MODCLI_012_CheckpointCommandsMapToProtocolCommands)
{
    FakeCommandTransport transport;
    grav_cmd::CommandSessionState session;
    std::ostringstream output;
    grav_cmd::CommandExecutionContext context{transport, session,
                                              grav_cmd::CommandExecutionMode::Batch, output};
    const grav_cmd::CommandResult saveResult = grav_cmd::CommandExecutor::execute(
        parseSingle("save_checkpoint checkpoints/demo.chk"), context);
    ASSERT_TRUE(saveResult.ok);
    ASSERT_EQ(transport.commandHistory.size(), 1u);
    EXPECT_EQ(transport.commandHistory[0].first, "save_checkpoint");
    EXPECT_EQ(transport.commandHistory[0].second, "\"path\":\"checkpoints/demo.chk\"");
    transport.commandHistory.clear();
    const grav_cmd::CommandResult loadResult = grav_cmd::CommandExecutor::execute(
        parseSingle("load_checkpoint checkpoints/demo.chk"), context);
    ASSERT_TRUE(loadResult.ok);
    ASSERT_EQ(transport.commandHistory.size(), 1u);
    EXPECT_EQ(transport.commandHistory[0].first, "load_checkpoint");
    EXPECT_EQ(transport.commandHistory[0].second, "\"path\":\"checkpoints/demo.chk\"");
}
} // namespace grav_test_module_cli_command_executor
