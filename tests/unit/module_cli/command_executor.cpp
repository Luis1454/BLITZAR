/*
 * @file tests/unit/module_cli/command_executor.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Automated verification assets for BLITZAR quality gates.
 */

#include "command/core/Context.hpp"
#include "command/execution/Executor.hpp"
#include "command/parsing/Parser.hpp"
#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace bltzr_test_module_cli_command_executor {
class FakeTransport final : public bltzr_cmd::Transport {
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

    bltzr_protocol::Response sendCommand(const std::string& cmd,
                                     const std::string& fieldsJson = "") override
    {
        commandHistory.emplace_back(cmd, fieldsJson);
        return nextCommandResponse;
    }

    bltzr_protocol::Response getStatus(bltzr_protocol::ClientStatus& outStatus) override
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
    bltzr_protocol::Response nextCommandResponse{true, {}, {}};
    bltzr_protocol::Response nextStatusResponse{true, {}, {}};
    std::vector<std::pair<std::string, std::uint16_t>> connectHistory;
    std::vector<std::pair<std::string, std::string>> commandHistory;
    std::vector<bltzr_protocol::ClientStatus> scriptedStatuses;
};

static bltzr_cmd::Request parseSingle(const std::string& line)
{
    const bltzr_cmd::ParseResult parsed = bltzr_cmd::parseLine(line, 1u);
    EXPECT_TRUE(parsed.ok);
    EXPECT_EQ(parsed.requests.size(), 1u);
    return parsed.requests.front();
}

TEST(ExecutorTest, TST_UNT_MODCLI_009_SetDtMapsToProtocolCommand)
{
    FakeTransport transport;
    bltzr_cmd::SessionState session;
    std::ostringstream output;
    bltzr_cmd::ExecutionContext context{
        transport, session, bltzr_cmd::ExecutionMode::Interactive, output};
    const bltzr_cmd::Result result =
        bltzr_cmd::execute(parseSingle("set_dt 0.25"), context);
    ASSERT_TRUE(result.ok);
    ASSERT_EQ(transport.commandHistory.size(), 1u);
    EXPECT_EQ(transport.commandHistory[0].first, "set_dt");
    EXPECT_EQ(transport.commandHistory[0].second, "\"value\":0.250000");
    EXPECT_FLOAT_EQ(session.config.dt, 0.25F);
}

TEST(ExecutorTest, TST_UNT_MODCLI_010_RunStepsAndRunUntilUseDeterministicStepFlow)
{
    FakeTransport transport;
    transport.connected = true;
    bltzr_protocol::ClientStatus initialStatus{};
    initialStatus.steps = 0u;
    initialStatus.totalTime = 0.0F;
    bltzr_protocol::ClientStatus middleStatus{};
    middleStatus.steps = 1u;
    middleStatus.totalTime = 0.5F;
    bltzr_protocol::ClientStatus finalStatus{};
    finalStatus.steps = 2u;
    finalStatus.totalTime = 1.0F;
    transport.scriptedStatuses = {initialStatus, middleStatus, finalStatus};
    bltzr_cmd::SessionState session;
    std::ostringstream output;
    bltzr_cmd::ExecutionContext context{transport, session,
                                               bltzr_cmd::ExecutionMode::Batch, output};
    const bltzr_cmd::Result runSteps =
        bltzr_cmd::execute(parseSingle("run_steps 3"), context);
    ASSERT_TRUE(runSteps.ok);
    ASSERT_EQ(transport.commandHistory.size(), 2u);
    EXPECT_EQ(transport.commandHistory[0].first, "pause");
    EXPECT_EQ(transport.commandHistory[1].first, "step");
    EXPECT_EQ(transport.commandHistory[1].second, "\"count\":3");
    transport.commandHistory.clear();
    const bltzr_cmd::Result runUntil =
        bltzr_cmd::execute(parseSingle("run_until 1.0"), context);
    ASSERT_TRUE(runUntil.ok);
    ASSERT_EQ(transport.commandHistory.size(), 3u);
    EXPECT_EQ(transport.commandHistory[0].first, "pause");
    EXPECT_EQ(transport.commandHistory[1].first, "step");
    EXPECT_EQ(transport.commandHistory[2].first, "step");
    EXPECT_EQ(transport.statusCallCount, 3u);
}

TEST(ExecutorTest, TST_UNT_MODCLI_011_LoadConfigUpdatesSessionPathAndRejectsInvalidProfile)
{
    FakeTransport transport;
    bltzr_cmd::SessionState session;
    session.configPath = "before.ini";
    std::ostringstream output;
    bltzr_cmd::ExecutionContext context{transport, session,
                                               bltzr_cmd::ExecutionMode::Batch, output};
    const bltzr_cmd::Result invalidProfile =
        bltzr_cmd::execute(parseSingle("set_profile impossible"), context);
    ASSERT_FALSE(invalidProfile.ok);
    EXPECT_EQ(invalidProfile.message, "invalid performance profile");
    const std::string originalPath = session.configPath;
    const bltzr_cmd::Result loadResult =
        bltzr_cmd::execute(parseSingle("load_config simulation.ini"), context);
    ASSERT_TRUE(loadResult.ok);
    EXPECT_EQ(session.configPath, "simulation.ini");
    EXPECT_NE(session.configPath, originalPath);
}

TEST(ExecutorTest, TST_UNT_MODCLI_012_CheckpointCommandsMapToProtocolCommands)
{
    FakeTransport transport;
    bltzr_cmd::SessionState session;
    std::ostringstream output;
    bltzr_cmd::ExecutionContext context{transport, session,
                                               bltzr_cmd::ExecutionMode::Batch, output};
    const bltzr_cmd::Result saveResult = bltzr_cmd::execute(
        parseSingle("save_checkpoint checkpoints/demo.chk"), context);
    ASSERT_TRUE(saveResult.ok);
    ASSERT_EQ(transport.commandHistory.size(), 1u);
    EXPECT_EQ(transport.commandHistory[0].first, "save_checkpoint");
    EXPECT_EQ(transport.commandHistory[0].second, "\"path\":\"checkpoints/demo.chk\"");
    transport.commandHistory.clear();
    const bltzr_cmd::Result loadResult = bltzr_cmd::execute(
        parseSingle("load_checkpoint checkpoints/demo.chk"), context);
    ASSERT_TRUE(loadResult.ok);
    ASSERT_EQ(transport.commandHistory.size(), 1u);
    EXPECT_EQ(transport.commandHistory[0].first, "load_checkpoint");
    EXPECT_EQ(transport.commandHistory[0].second, "\"path\":\"checkpoints/demo.chk\"");
}
} // namespace bltzr_test_module_cli_command_executor
