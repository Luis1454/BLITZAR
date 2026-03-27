#include "command/CommandContext.hpp"
#include "command/CommandExecutor.hpp"
#include "command/CommandParser.hpp"

#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace grav_test_module_cli_command_executor_flows {

class FakeCommandTransport final : public grav_cmd::CommandTransport {
public:
    bool connect(const std::string &host, std::uint16_t port) override
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

    ServerClientResponse sendCommand(const std::string &cmd, const std::string &fieldsJson = "") override
    {
        commandHistory.emplace_back(cmd, fieldsJson);
        return nextCommandResponse;
    }

    ServerClientResponse getStatus(ServerClientStatus &outStatus) override
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

static grav_cmd::CommandRequest parseSingle(const std::string &line)
{
    const grav_cmd::CommandParseResult parsed = grav_cmd::CommandParser::parseLine(line, 1u);
    EXPECT_TRUE(parsed.ok);
    EXPECT_EQ(parsed.requests.size(), 1u);
    return parsed.requests.front();
}

TEST(CommandExecutorFlowTest, TST_UNT_MODCLI_013_HelpRendersCatalogAndReturnsSuccess)
{
    FakeCommandTransport transport;
    grav_cmd::CommandSessionState session;
    std::ostringstream output;
    grav_cmd::CommandExecutionContext context{transport, session, grav_cmd::CommandExecutionMode::Interactive, output};

    const grav_cmd::CommandResult result = grav_cmd::CommandExecutor::execute(parseSingle("help"), context);
    ASSERT_TRUE(result.ok);
    EXPECT_NE(output.str().find("commands:"), std::string::npos);
    EXPECT_NE(output.str().find("load_config"), std::string::npos);
    EXPECT_TRUE(transport.commandHistory.empty());
}

TEST(CommandExecutorFlowTest, TST_UNT_MODCLI_014_ConnectionCommandsHandleSuccessAndFailure)
{
    FakeCommandTransport transport;
    grav_cmd::CommandSessionState session;
    std::ostringstream output;
    grav_cmd::CommandExecutionContext context{transport, session, grav_cmd::CommandExecutionMode::Interactive, output};

    const grav_cmd::CommandResult connectResult = grav_cmd::CommandExecutor::execute(parseSingle("connect 192.168.1.10 5000"), context);
    ASSERT_TRUE(connectResult.ok);
    EXPECT_EQ(connectResult.message, "connected");
    EXPECT_TRUE(transport.disconnected);
    ASSERT_EQ(transport.connectHistory.size(), 1u);
    EXPECT_EQ(transport.connectHistory[0].first, "192.168.1.10");
    EXPECT_EQ(transport.connectHistory[0].second, 5000u);

    transport.connectResult = false;
    const grav_cmd::CommandResult reconnectResult = grav_cmd::CommandExecutor::execute(parseSingle("reconnect"), context);
    ASSERT_FALSE(reconnectResult.ok);
    EXPECT_EQ(reconnectResult.message, "reconnect failed");
}

TEST(CommandExecutorFlowTest, TST_UNT_MODCLI_015_ControlCommandsMapToProtocol)
{
    FakeCommandTransport transport;
    transport.connected = true;
    grav_cmd::CommandSessionState session;
    std::ostringstream output;
    grav_cmd::CommandExecutionContext context{transport, session, grav_cmd::CommandExecutionMode::Batch, output};

    ASSERT_TRUE(grav_cmd::CommandExecutor::execute(parseSingle("pause"), context).ok);
    ASSERT_TRUE(grav_cmd::CommandExecutor::execute(parseSingle("resume"), context).ok);
    ASSERT_TRUE(grav_cmd::CommandExecutor::execute(parseSingle("toggle"), context).ok);
    ASSERT_TRUE(grav_cmd::CommandExecutor::execute(parseSingle("step"), context).ok);
    ASSERT_TRUE(grav_cmd::CommandExecutor::execute(parseSingle("reset"), context).ok);
    ASSERT_TRUE(grav_cmd::CommandExecutor::execute(parseSingle("recover"), context).ok);
    ASSERT_TRUE(grav_cmd::CommandExecutor::execute(parseSingle("shutdown"), context).ok);
    ASSERT_TRUE(grav_cmd::CommandExecutor::execute(parseSingle("set_particle_count 42"), context).ok);

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
    grav_cmd::CommandExecutionContext context{transport, session, grav_cmd::CommandExecutionMode::Interactive, output};

    const grav_cmd::CommandResult okStatus = grav_cmd::CommandExecutor::execute(parseSingle("status"), context);
    ASSERT_TRUE(okStatus.ok);
    EXPECT_NE(output.str().find("RUNNING step=12"), std::string::npos);

    transport.nextStatusResponse = ServerClientResponse{false, {}, {}};
    const grav_cmd::CommandResult failedStatus = grav_cmd::CommandExecutor::execute(parseSingle("status"), context);
    ASSERT_FALSE(failedStatus.ok);
    EXPECT_EQ(failedStatus.message, "status failed");
}

TEST(CommandExecutorFlowTest, TST_UNT_MODCLI_017_RunUntilAndSendCheckedFailurePaths)
{
    FakeCommandTransport transport;
    grav_cmd::CommandSessionState session;
    std::ostringstream output;
    grav_cmd::CommandExecutionContext context{transport, session, grav_cmd::CommandExecutionMode::Batch, output};

    const grav_cmd::CommandResult negative = grav_cmd::CommandExecutor::execute(parseSingle("run_until -1"), context);
    ASSERT_FALSE(negative.ok);
    EXPECT_EQ(negative.message, "run_until requires a non-negative simulation time");

    transport.connectResult = false;
    transport.connected = false;
    const grav_cmd::CommandResult noConnect = grav_cmd::CommandExecutor::execute(parseSingle("pause"), context);
    ASSERT_FALSE(noConnect.ok);
    EXPECT_NE(noConnect.message.find("unable to connect to server"), std::string::npos);

    transport.connectResult = true;
    transport.connected = true;
    transport.nextCommandResponse = ServerClientResponse{false, {}, {}};
    const grav_cmd::CommandResult commandFailure = grav_cmd::CommandExecutor::execute(parseSingle("resume"), context);
    ASSERT_FALSE(commandFailure.ok);
    EXPECT_EQ(commandFailure.message, "server command failed");
}

TEST(CommandExecutorFlowTest, TST_UNT_MODCLI_018_SetModeCommandsValidateAndMap)
{
    FakeCommandTransport transport;
    transport.connected = true;
    grav_cmd::CommandSessionState session;
    std::ostringstream output;
    grav_cmd::CommandExecutionContext context{transport, session, grav_cmd::CommandExecutionMode::Interactive, output};

    const grav_cmd::CommandResult badSolver = grav_cmd::CommandExecutor::execute(parseSingle("set_solver impossible"), context);
    ASSERT_FALSE(badSolver.ok);
    EXPECT_EQ(badSolver.message, "invalid solver value");

    const grav_cmd::CommandResult badIntegrator = grav_cmd::CommandExecutor::execute(parseSingle("set_integrator impossible"), context);
    ASSERT_FALSE(badIntegrator.ok);
    EXPECT_EQ(badIntegrator.message, "invalid integrator value");

    const grav_cmd::CommandResult solver = grav_cmd::CommandExecutor::execute(parseSingle("set_solver octree_cpu"), context);
    ASSERT_TRUE(solver.ok);
    const grav_cmd::CommandResult integrator = grav_cmd::CommandExecutor::execute(parseSingle("set_integrator rk4"), context);
    ASSERT_TRUE(integrator.ok);

    ASSERT_EQ(transport.commandHistory.size(), 2u);
    EXPECT_EQ(transport.commandHistory[0].first, "set_solver");
    EXPECT_EQ(transport.commandHistory[0].second, "\"value\":\"octree_cpu\"");
    EXPECT_EQ(transport.commandHistory[1].first, "set_integrator");
    EXPECT_EQ(transport.commandHistory[1].second, "\"value\":\"rk4\"");
}

TEST(CommandExecutorFlowTest, TST_UNT_MODCLI_019_ExportSnapshotUsesDerivedAndExplicitFormats)
{
    FakeCommandTransport transport;
    transport.connected = true;
    grav_cmd::CommandSessionState session;
    std::ostringstream output;
    grav_cmd::CommandExecutionContext context{transport, session, grav_cmd::CommandExecutionMode::Interactive, output};

    const grav_cmd::CommandResult derived = grav_cmd::CommandExecutor::execute(parseSingle("export_snapshot outputs/frame.xyz"), context);
    ASSERT_TRUE(derived.ok);

    const grav_cmd::CommandResult explicitFormat = grav_cmd::CommandExecutor::execute(parseSingle("export_snapshot outputs/frame.bin vtk_binary"), context);
    ASSERT_TRUE(explicitFormat.ok);

    ASSERT_EQ(transport.commandHistory.size(), 2u);
    EXPECT_EQ(transport.commandHistory[0].first, "export");
    EXPECT_NE(transport.commandHistory[0].second.find("\"format\":\"xyz\""), std::string::npos);
    EXPECT_EQ(transport.commandHistory[1].first, "export");
    EXPECT_NE(transport.commandHistory[1].second.find("\"format\":\"vtk_binary\""), std::string::npos);
}

} // namespace grav_test_module_cli_command_executor_flows
