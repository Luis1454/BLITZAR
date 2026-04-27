// File: tests/unit/module_cli/command_executor_flows.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "command/CommandContext.hpp"
#include "command/CommandExecutor.hpp"
#include "command/CommandParser.hpp"
#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
namespace grav_test_module_cli_command_executor_flows {
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
TEST(CommandExecutorFlowTest, TST_UNT_MODCLI_013_HelpRendersCatalogAndReturnsSuccess)
{
    FakeCommandTransport transport;
    grav_cmd::CommandSessionState session;
    std::ostringstream output;
    grav_cmd::CommandExecutionContext context{transport, session,
                                              grav_cmd::CommandExecutionMode::Interactive, output};
    const grav_cmd::CommandResult result =
        /// Description: Executes the execute operation.
        grav_cmd::CommandExecutor::execute(parseSingle("help"), context);
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(result.ok);
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(output.str().find("commands:"), std::string::npos);
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(output.str().find("load_config"), std::string::npos);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(transport.commandHistory.empty());
}
/// Description: Executes the TEST operation.
TEST(CommandExecutorFlowTest, TST_UNT_MODCLI_014_ConnectionCommandsHandleSuccessAndFailure)
{
    FakeCommandTransport transport;
    grav_cmd::CommandSessionState session;
    std::ostringstream output;
    grav_cmd::CommandExecutionContext context{transport, session,
                                              grav_cmd::CommandExecutionMode::Interactive, output};
    const grav_cmd::CommandResult connectResult =
        /// Description: Executes the execute operation.
        grav_cmd::CommandExecutor::execute(parseSingle("connect 192.168.1.10 5000"), context);
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(connectResult.ok);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(connectResult.message, "connected");
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(transport.disconnected);
    /// Description: Executes the ASSERT_EQ operation.
    ASSERT_EQ(transport.connectHistory.size(), 1u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(transport.connectHistory[0].first, "192.168.1.10");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(transport.connectHistory[0].second, 5000u);
    transport.connectResult = false;
    const grav_cmd::CommandResult reconnectResult =
        /// Description: Executes the execute operation.
        grav_cmd::CommandExecutor::execute(parseSingle("reconnect"), context);
    /// Description: Executes the ASSERT_FALSE operation.
    ASSERT_FALSE(reconnectResult.ok);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(reconnectResult.message, "reconnect failed");
}
/// Description: Executes the TEST operation.
TEST(CommandExecutorFlowTest, TST_UNT_MODCLI_015_ControlCommandsMapToProtocol)
{
    FakeCommandTransport transport;
    transport.connected = true;
    grav_cmd::CommandSessionState session;
    std::ostringstream output;
    grav_cmd::CommandExecutionContext context{transport, session,
                                              grav_cmd::CommandExecutionMode::Batch, output};
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(grav_cmd::CommandExecutor::execute(parseSingle("pause"), context).ok);
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(grav_cmd::CommandExecutor::execute(parseSingle("resume"), context).ok);
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(grav_cmd::CommandExecutor::execute(parseSingle("toggle"), context).ok);
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(grav_cmd::CommandExecutor::execute(parseSingle("step"), context).ok);
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(grav_cmd::CommandExecutor::execute(parseSingle("reset"), context).ok);
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(grav_cmd::CommandExecutor::execute(parseSingle("recover"), context).ok);
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(grav_cmd::CommandExecutor::execute(parseSingle("shutdown"), context).ok);
    ASSERT_TRUE(
        /// Description: Executes the execute operation.
        grav_cmd::CommandExecutor::execute(parseSingle("set_particle_count 42"), context).ok);
    /// Description: Executes the ASSERT_EQ operation.
    ASSERT_EQ(transport.commandHistory.size(), 8u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(transport.commandHistory[0].first, "pause");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(transport.commandHistory[1].first, "resume");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(transport.commandHistory[2].first, "toggle");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(transport.commandHistory[3].first, "step");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(transport.commandHistory[3].second, "\"count\":1");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(transport.commandHistory[4].first, "reset");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(transport.commandHistory[5].first, "recover");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(transport.commandHistory[6].first, "shutdown");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(transport.commandHistory[7].first, "set_particle_count");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(transport.commandHistory[7].second, "\"value\":42");
}
/// Description: Executes the TEST operation.
TEST(CommandExecutorFlowTest, TST_UNT_MODCLI_016_StatusReportsStateAndHandlesStatusFailure)
{
    FakeCommandTransport transport;
    transport.connected = true;
    ServerClientStatus status{};
    status.paused = false;
    status.faulted = false;
    status.steps = 12u;
    status.dt = 0.02f;
    status.solver = "octree_gpu";
    status.integrator = "rk4";
    status.performanceProfile = "balanced";
    status.totalTime = 2.5f;
    status.totalEnergy = 10.0f;
    status.energyDriftPct = 0.1f;
    transport.scriptedStatuses = {status};
    grav_cmd::CommandSessionState session;
    std::ostringstream output;
    grav_cmd::CommandExecutionContext context{transport, session,
                                              grav_cmd::CommandExecutionMode::Interactive, output};
    const grav_cmd::CommandResult okStatus =
        /// Description: Executes the execute operation.
        grav_cmd::CommandExecutor::execute(parseSingle("status"), context);
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(okStatus.ok);
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(output.str().find("RUNNING step=12"), std::string::npos);
    transport.nextStatusResponse = ServerClientResponse{false, {}, {}};
    const grav_cmd::CommandResult failedStatus =
        /// Description: Executes the execute operation.
        grav_cmd::CommandExecutor::execute(parseSingle("status"), context);
    /// Description: Executes the ASSERT_FALSE operation.
    ASSERT_FALSE(failedStatus.ok);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(failedStatus.message, "status failed");
}
/// Description: Executes the TEST operation.
TEST(CommandExecutorFlowTest, TST_UNT_MODCLI_017_RunUntilAndSendCheckedFailurePaths)
{
    FakeCommandTransport transport;
    grav_cmd::CommandSessionState session;
    std::ostringstream output;
    grav_cmd::CommandExecutionContext context{transport, session,
                                              grav_cmd::CommandExecutionMode::Batch, output};
    const grav_cmd::CommandResult negative =
        /// Description: Executes the execute operation.
        grav_cmd::CommandExecutor::execute(parseSingle("run_until -1"), context);
    /// Description: Executes the ASSERT_FALSE operation.
    ASSERT_FALSE(negative.ok);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(negative.message, "run_until requires a non-negative simulation time");
    transport.connectResult = false;
    transport.connected = false;
    const grav_cmd::CommandResult noConnect =
        /// Description: Executes the execute operation.
        grav_cmd::CommandExecutor::execute(parseSingle("pause"), context);
    /// Description: Executes the ASSERT_FALSE operation.
    ASSERT_FALSE(noConnect.ok);
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(noConnect.message.find("unable to connect to server"), std::string::npos);
    transport.connectResult = true;
    transport.connected = true;
    transport.nextCommandResponse = ServerClientResponse{false, {}, {}};
    const grav_cmd::CommandResult commandFailure =
        /// Description: Executes the execute operation.
        grav_cmd::CommandExecutor::execute(parseSingle("resume"), context);
    /// Description: Executes the ASSERT_FALSE operation.
    ASSERT_FALSE(commandFailure.ok);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(commandFailure.message, "server command failed");
}
/// Description: Executes the TEST operation.
TEST(CommandExecutorFlowTest, TST_UNT_MODCLI_018_SetModeCommandsValidateAndMap)
{
    FakeCommandTransport transport;
    transport.connected = true;
    grav_cmd::CommandSessionState session;
    std::ostringstream output;
    grav_cmd::CommandExecutionContext context{transport, session,
                                              grav_cmd::CommandExecutionMode::Interactive, output};
    const grav_cmd::CommandResult badSolver =
        /// Description: Executes the execute operation.
        grav_cmd::CommandExecutor::execute(parseSingle("set_solver impossible"), context);
    /// Description: Executes the ASSERT_FALSE operation.
    ASSERT_FALSE(badSolver.ok);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(badSolver.message, "invalid solver value");
    const grav_cmd::CommandResult badIntegrator =
        /// Description: Executes the execute operation.
        grav_cmd::CommandExecutor::execute(parseSingle("set_integrator impossible"), context);
    /// Description: Executes the ASSERT_FALSE operation.
    ASSERT_FALSE(badIntegrator.ok);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(badIntegrator.message, "invalid integrator value");
    const grav_cmd::CommandResult solver =
        /// Description: Executes the execute operation.
        grav_cmd::CommandExecutor::execute(parseSingle("set_solver octree_cpu"), context);
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(solver.ok);
    const grav_cmd::CommandResult integrator =
        /// Description: Executes the execute operation.
        grav_cmd::CommandExecutor::execute(parseSingle("set_integrator rk4"), context);
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(integrator.ok);
    /// Description: Executes the ASSERT_EQ operation.
    ASSERT_EQ(transport.commandHistory.size(), 2u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(transport.commandHistory[0].first, "set_solver");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(transport.commandHistory[0].second, "\"value\":\"octree_cpu\"");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(transport.commandHistory[1].first, "set_integrator");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(transport.commandHistory[1].second, "\"value\":\"rk4\"");
}
/// Description: Executes the TEST operation.
TEST(CommandExecutorFlowTest, TST_UNT_MODCLI_019_ExportSnapshotUsesDerivedAndExplicitFormats)
{
    FakeCommandTransport transport;
    transport.connected = true;
    grav_cmd::CommandSessionState session;
    std::ostringstream output;
    grav_cmd::CommandExecutionContext context{transport, session,
                                              grav_cmd::CommandExecutionMode::Interactive, output};
    const grav_cmd::CommandResult derived = grav_cmd::CommandExecutor::execute(
        /// Description: Executes the parseSingle operation.
        parseSingle("export_snapshot outputs/frame.xyz"), context);
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(derived.ok);
    const grav_cmd::CommandResult explicitFormat = grav_cmd::CommandExecutor::execute(
        /// Description: Executes the parseSingle operation.
        parseSingle("export_snapshot outputs/frame.bin vtk_binary"), context);
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(explicitFormat.ok);
    /// Description: Executes the ASSERT_EQ operation.
    ASSERT_EQ(transport.commandHistory.size(), 2u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(transport.commandHistory[0].first, "export");
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(transport.commandHistory[0].second.find("\"format\":\"xyz\""), std::string::npos);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(transport.commandHistory[1].first, "export");
    EXPECT_NE(transport.commandHistory[1].second.find("\"format\":\"vtk_binary\""),
              std::string::npos);
}
/// Description: Executes the TEST operation.
TEST(CommandExecutorFlowTest, TST_UNT_MODCLI_037_RunUntilPropagatesStatusErrorDetails)
{
    FakeCommandTransport transport;
    transport.connected = true;
    transport.nextStatusResponse = ServerClientResponse{false, {}, "status timeout"};
    grav_cmd::CommandSessionState session;
    std::ostringstream output;
    grav_cmd::CommandExecutionContext context{transport, session,
                                              grav_cmd::CommandExecutionMode::Batch, output};
    const grav_cmd::CommandResult result =
        /// Description: Executes the execute operation.
        grav_cmd::CommandExecutor::execute(parseSingle("run_until 5.0"), context);
    /// Description: Executes the ASSERT_FALSE operation.
    ASSERT_FALSE(result.ok);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(result.message, "status timeout");
    /// Description: Executes the ASSERT_FALSE operation.
    ASSERT_FALSE(transport.commandHistory.empty());
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(transport.commandHistory[0].first, "pause");
}
/// Description: Executes the TEST operation.
TEST(CommandExecutorFlowTest, TST_UNT_MODCLI_038_SendCheckedReturnsServerErrorDetail)
{
    FakeCommandTransport transport;
    transport.connected = true;
    transport.nextCommandResponse = ServerClientResponse{false, {}, "permission denied"};
    grav_cmd::CommandSessionState session;
    std::ostringstream output;
    grav_cmd::CommandExecutionContext context{transport, session,
                                              grav_cmd::CommandExecutionMode::Interactive, output};
    const grav_cmd::CommandResult result =
        /// Description: Executes the execute operation.
        grav_cmd::CommandExecutor::execute(parseSingle("resume"), context);
    /// Description: Executes the ASSERT_FALSE operation.
    ASSERT_FALSE(result.ok);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(result.message, "permission denied");
    /// Description: Executes the ASSERT_EQ operation.
    ASSERT_EQ(transport.commandHistory.size(), 1u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(transport.commandHistory[0].first, "resume");
}
/// Description: Executes the TEST operation.
TEST(CommandExecutorFlowTest, TST_UNT_MODCLI_039_RunUntilReturnsImmediatelyWhenTargetAlreadyReached)
{
    FakeCommandTransport transport;
    transport.connected = true;
    ServerClientStatus status{};
    status.totalTime = 6.0f;
    transport.scriptedStatuses = {status};
    grav_cmd::CommandSessionState session;
    std::ostringstream output;
    grav_cmd::CommandExecutionContext context{transport, session,
                                              grav_cmd::CommandExecutionMode::Batch, output};
    const grav_cmd::CommandResult result =
        /// Description: Executes the execute operation.
        grav_cmd::CommandExecutor::execute(parseSingle("run_until 5.0"), context);
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(result.ok);
    /// Description: Executes the ASSERT_FALSE operation.
    ASSERT_FALSE(transport.commandHistory.empty());
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(transport.commandHistory[0].first, "pause");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(transport.commandHistory.size(), 1u);
}
} // namespace grav_test_module_cli_command_executor_flows
