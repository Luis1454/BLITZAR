/*
 * @file tests/unit/module_cli/command_executor_flows.cpp
 * @author BLITZAR Contributors
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
#include "Constants.hpp"

namespace bltzr_test_module_cli_command_executor_flows {
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

TEST(ExecutorFlowTest, TST_UNT_MODCLI_013_HelpRendersCatalogAndReturnsSuccess)
{
    FakeTransport transport;
    bltzr_cmd::SessionState session;
    std::ostringstream output;
    bltzr_cmd::ExecutionContext context{
        transport, session, bltzr_cmd::ExecutionMode::Interactive, output};
    const bltzr_cmd::Result result =
        bltzr_cmd::execute(parseSingle("help"), context);
    ASSERT_TRUE(result.ok);
    EXPECT_NE(output.str().find("commands:"), std::string::npos);
    EXPECT_NE(output.str().find("load_config"), std::string::npos);
    EXPECT_TRUE(transport.commandHistory.empty());
}

TEST(ExecutorFlowTest, TST_UNT_MODCLI_014_ConnectionCommandsHandleSuccessAndFailure)
{
    FakeTransport transport;
    bltzr_cmd::SessionState session;
    std::ostringstream output;
    bltzr_cmd::ExecutionContext context{
        transport, session, bltzr_cmd::ExecutionMode::Interactive, output};
    const bltzr_cmd::Result connectResult =
        bltzr_cmd::execute(parseSingle("connect 192.168.1.10 5000"), context);
    ASSERT_TRUE(connectResult.ok);
    EXPECT_EQ(connectResult.message, "connected");
    EXPECT_TRUE(transport.disconnected);
    ASSERT_EQ(transport.connectHistory.size(), 1u);
    EXPECT_EQ(transport.connectHistory[0].first, "192.168.1.10");
    EXPECT_EQ(transport.connectHistory[0].second, 5000u);
    transport.connectResult = false;
    const bltzr_cmd::Result reconnectResult =
        bltzr_cmd::execute(parseSingle("reconnect"), context);
    ASSERT_FALSE(reconnectResult.ok);
    EXPECT_EQ(reconnectResult.message, "reconnect failed");
}

TEST(ExecutorFlowTest, TST_UNT_MODCLI_015_ControlCommandsMapToProtocol)
{
    FakeTransport transport;
    transport.connected = true;
    bltzr_cmd::SessionState session;
    std::ostringstream output;
    bltzr_cmd::ExecutionContext context{transport, session,
                                               bltzr_cmd::ExecutionMode::Batch, output};
    ASSERT_TRUE(bltzr_cmd::execute(parseSingle("pause"), context).ok);
    ASSERT_TRUE(bltzr_cmd::execute(parseSingle("resume"), context).ok);
    ASSERT_TRUE(bltzr_cmd::execute(parseSingle("toggle"), context).ok);
    ASSERT_TRUE(bltzr_cmd::execute(parseSingle("step"), context).ok);
    ASSERT_TRUE(bltzr_cmd::execute(parseSingle("reset"), context).ok);
    ASSERT_TRUE(bltzr_cmd::execute(parseSingle("recover"), context).ok);
    ASSERT_TRUE(bltzr_cmd::execute(parseSingle("shutdown"), context).ok);
    ASSERT_TRUE(
        bltzr_cmd::execute(parseSingle("set_particle_count 42"), context).ok);
    ASSERT_EQ(transport.commandHistory.size(), 8u);
    EXPECT_EQ(transport.commandHistory[0].first, "pause");
    EXPECT_EQ(transport.commandHistory[1].first, "resume");
    EXPECT_EQ(transport.commandHistory[2].first, "toggle");
    EXPECT_EQ(transport.commandHistory[3].first, "step");
    EXPECT_EQ(transport.commandHistory[3].second, "\"count\":1");
    EXPECT_EQ(transport.commandHistory[4].first, "reset");
    EXPECT_EQ(transport.commandHistory[5].first, "recover");
    EXPECT_EQ(transport.commandHistory[6].first, "shutdown");
    EXPECT_EQ(transport.commandHistory[7].first, "set_particle_count");
    EXPECT_EQ(transport.commandHistory[7].second, "\"value\":42");
}

TEST(ExecutorFlowTest, TST_UNT_MODCLI_016_StatusReportsStateAndHandlesStatusFailure)
{
    FakeTransport transport;
    transport.connected = true;
    bltzr_protocol::ClientStatus status{};
    status.paused = false;
    status.faulted = false;
    status.steps = 12u;
    status.dt = kDefaultSimulationDt;
    status.solver = "octree_gpu";
    status.integrator = "rk4";
    status.performanceProfile = "balanced";
    status.totalTime = 2.5f;
    status.totalEnergy = 10.0f;
    status.energyDriftPct = 0.1f;
    transport.scriptedStatuses = {status};
    bltzr_cmd::SessionState session;
    std::ostringstream output;
    bltzr_cmd::ExecutionContext context{
        transport, session, bltzr_cmd::ExecutionMode::Interactive, output};
    const bltzr_cmd::Result okStatus =
        bltzr_cmd::execute(parseSingle("status"), context);
    ASSERT_TRUE(okStatus.ok);
    EXPECT_NE(output.str().find("RUNNING step=12"), std::string::npos);
    transport.nextStatusResponse = bltzr_protocol::Response{false, {}, {}};
    const bltzr_cmd::Result failedStatus =
        bltzr_cmd::execute(parseSingle("status"), context);
    ASSERT_FALSE(failedStatus.ok);
    EXPECT_EQ(failedStatus.message, "status failed");
}

TEST(ExecutorFlowTest, TST_UNT_MODCLI_017_RunUntilAndSendCheckedFailurePaths)
{
    FakeTransport transport;
    bltzr_cmd::SessionState session;
    std::ostringstream output;
    bltzr_cmd::ExecutionContext context{transport, session,
                                               bltzr_cmd::ExecutionMode::Batch, output};
    const bltzr_cmd::Result negative =
        bltzr_cmd::execute(parseSingle("run_until -1"), context);
    ASSERT_FALSE(negative.ok);
    EXPECT_EQ(negative.message, "run_until requires a non-negative simulation time");
    transport.connectResult = false;
    transport.connected = false;
    const bltzr_cmd::Result noConnect =
        bltzr_cmd::execute(parseSingle("pause"), context);
    ASSERT_FALSE(noConnect.ok);
    EXPECT_NE(noConnect.message.find("unable to connect to server"), std::string::npos);
    transport.connectResult = true;
    transport.connected = true;
    transport.nextCommandResponse = bltzr_protocol::Response{false, {}, {}};
    const bltzr_cmd::Result commandFailure =
        bltzr_cmd::execute(parseSingle("resume"), context);
    ASSERT_FALSE(commandFailure.ok);
    EXPECT_EQ(commandFailure.message, "server command failed");
}

TEST(ExecutorFlowTest, TST_UNT_MODCLI_018_SetModeCommandsValidateAndMap)
{
    FakeTransport transport;
    transport.connected = true;
    bltzr_cmd::SessionState session;
    std::ostringstream output;
    bltzr_cmd::ExecutionContext context{
        transport, session, bltzr_cmd::ExecutionMode::Interactive, output};
    const bltzr_cmd::Result badSolver =
        bltzr_cmd::execute(parseSingle("set_solver impossible"), context);
    ASSERT_FALSE(badSolver.ok);
    EXPECT_EQ(badSolver.message, "invalid solver value");
    const bltzr_cmd::Result badIntegrator =
        bltzr_cmd::execute(parseSingle("set_integrator impossible"), context);
    ASSERT_FALSE(badIntegrator.ok);
    EXPECT_EQ(badIntegrator.message, "invalid integrator value");
    const bltzr_cmd::Result solver =
        bltzr_cmd::execute(parseSingle("set_solver octree_cpu"), context);
    ASSERT_TRUE(solver.ok);
    const bltzr_cmd::Result integrator =
        bltzr_cmd::execute(parseSingle("set_integrator rk4"), context);
    ASSERT_TRUE(integrator.ok);
    ASSERT_EQ(transport.commandHistory.size(), 2u);
    EXPECT_EQ(transport.commandHistory[0].first, "set_solver");
    EXPECT_EQ(transport.commandHistory[0].second, "\"value\":\"octree_cpu\"");
    EXPECT_EQ(transport.commandHistory[1].first, "set_integrator");
    EXPECT_EQ(transport.commandHistory[1].second, "\"value\":\"rk4\"");
}

TEST(ExecutorFlowTest, TST_UNT_MODCLI_019_ExportSnapshotUsesDerivedAndExplicitFormats)
{
    FakeTransport transport;
    transport.connected = true;
    bltzr_cmd::SessionState session;
    std::ostringstream output;
    bltzr_cmd::ExecutionContext context{
        transport, session, bltzr_cmd::ExecutionMode::Interactive, output};
    const bltzr_cmd::Result derived = bltzr_cmd::execute(
        parseSingle("export_snapshot outputs/frame.xyz"), context);
    ASSERT_TRUE(derived.ok);
    const bltzr_cmd::Result explicitFormat = bltzr_cmd::execute(
        parseSingle("export_snapshot outputs/frame.bin vtk_binary"), context);
    ASSERT_TRUE(explicitFormat.ok);
    ASSERT_EQ(transport.commandHistory.size(), 2u);
    EXPECT_EQ(transport.commandHistory[0].first, "export");
    EXPECT_NE(transport.commandHistory[0].second.find("\"format\":\"xyz\""), std::string::npos);
    EXPECT_EQ(transport.commandHistory[1].first, "export");
    EXPECT_NE(transport.commandHistory[1].second.find("\"format\":\"vtk_binary\""),
              std::string::npos);
}

TEST(ExecutorFlowTest, TST_UNT_MODCLI_037_RunUntilPropagatesStatusErrorDetails)
{
    FakeTransport transport;
    transport.connected = true;
    transport.nextStatusResponse = bltzr_protocol::Response{false, {}, "status timeout"};
    bltzr_cmd::SessionState session;
    std::ostringstream output;
    bltzr_cmd::ExecutionContext context{transport, session,
                                               bltzr_cmd::ExecutionMode::Batch, output};
    const bltzr_cmd::Result result =
        bltzr_cmd::execute(parseSingle("run_until 5.0"), context);
    ASSERT_FALSE(result.ok);
    EXPECT_EQ(result.message, "status timeout");
    ASSERT_FALSE(transport.commandHistory.empty());
    EXPECT_EQ(transport.commandHistory[0].first, "pause");
}

TEST(ExecutorFlowTest, TST_UNT_MODCLI_038_SendCheckedReturnsServerErrorDetail)
{
    FakeTransport transport;
    transport.connected = true;
    transport.nextCommandResponse = bltzr_protocol::Response{false, {}, "permission denied"};
    bltzr_cmd::SessionState session;
    std::ostringstream output;
    bltzr_cmd::ExecutionContext context{
        transport, session, bltzr_cmd::ExecutionMode::Interactive, output};
    const bltzr_cmd::Result result =
        bltzr_cmd::execute(parseSingle("resume"), context);
    ASSERT_FALSE(result.ok);
    EXPECT_EQ(result.message, "permission denied");
    ASSERT_EQ(transport.commandHistory.size(), 1u);
    EXPECT_EQ(transport.commandHistory[0].first, "resume");
}

TEST(ExecutorFlowTest, TST_UNT_MODCLI_039_RunUntilReturnsImmediatelyWhenTargetAlreadyReached)
{
    FakeTransport transport;
    transport.connected = true;
    bltzr_protocol::ClientStatus status{};
    status.totalTime = 6.0f;
    transport.scriptedStatuses = {status};
    bltzr_cmd::SessionState session;
    std::ostringstream output;
    bltzr_cmd::ExecutionContext context{transport, session,
                                               bltzr_cmd::ExecutionMode::Batch, output};
    const bltzr_cmd::Result result =
        bltzr_cmd::execute(parseSingle("run_until 5.0"), context);
    ASSERT_TRUE(result.ok);
    ASSERT_FALSE(transport.commandHistory.empty());
    EXPECT_EQ(transport.commandHistory[0].first, "pause");
    EXPECT_EQ(transport.commandHistory.size(), 1u);
}
} // namespace bltzr_test_module_cli_command_executor_flows
