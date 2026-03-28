#include "command/CommandContext.hpp"
#include "command/CommandExecutor.hpp"
#include "command/CommandParser.hpp"

#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace grav_test_module_cli_command_executor_flows_profile_export {

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

TEST(CommandExecutorFlowProfileExportTest, TST_UNT_MODCLI_040_SetProfileValidAppliesConfigWithoutReset)
{
    FakeCommandTransport transport;
    transport.connected = true;

    grav_cmd::CommandSessionState session;
    session.config.initConfigStyle = "preset";
    session.config.presetStructure = "disk_orbit";
    session.config.inputFile.clear();

    std::ostringstream output;
    grav_cmd::CommandExecutionContext context{transport, session, grav_cmd::CommandExecutionMode::Batch, output};

    const grav_cmd::CommandResult result = grav_cmd::CommandExecutor::execute(parseSingle("set_profile balanced"), context);
    ASSERT_TRUE(result.ok);
    EXPECT_EQ(session.config.performanceProfile, "balanced");
    EXPECT_EQ(result.message, "config applied");
    ASSERT_FALSE(transport.commandHistory.empty());
    EXPECT_EQ(transport.commandHistory.front().first, "set_particle_count");

    bool sawReset = false;
    bool sawEnergyMeasure = false;
    for (const auto &entry : transport.commandHistory) {
        if (entry.first == "reset") {
            sawReset = true;
        }
        if (entry.first == "set_energy_measure") {
            sawEnergyMeasure = true;
        }
    }
    EXPECT_FALSE(sawReset);
    EXPECT_TRUE(sawEnergyMeasure);
}

TEST(CommandExecutorFlowProfileExportTest, TST_UNT_MODCLI_041_ExportSnapshotWithoutExtensionUsesSessionDefaultFormat)
{
    FakeCommandTransport transport;
    transport.connected = true;

    grav_cmd::CommandSessionState session;
    session.config.exportFormat = "vtk_binary";

    std::ostringstream output;
    grav_cmd::CommandExecutionContext context{transport, session, grav_cmd::CommandExecutionMode::Interactive, output};

    const grav_cmd::CommandResult result = grav_cmd::CommandExecutor::execute(parseSingle("export_snapshot outputs/frame"), context);
    ASSERT_TRUE(result.ok);
    ASSERT_EQ(transport.commandHistory.size(), 1u);
    EXPECT_EQ(transport.commandHistory[0].first, "export");
    EXPECT_NE(transport.commandHistory[0].second.find("\"format\":\"vtk_binary\""), std::string::npos);
}

} // namespace grav_test_module_cli_command_executor_flows_profile_export
