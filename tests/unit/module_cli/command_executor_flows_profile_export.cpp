/*
 * @file tests/unit/module_cli/command_executor_flows_profile_export.cpp
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

namespace bltzr_test_module_cli_command_executor_flows_profile_export {
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

TEST(ExecutorFlowTest, TST_UNT_MODCLI_040_SetProfileValidAppliesConfigWithoutReset)
{
    FakeTransport transport;
    transport.connected = true;
    bltzr_cmd::SessionState session;
    session.config.initConfigStyle = "preset";
    session.config.presetStructure = "disk_orbit";
    session.config.inputFile.clear();
    std::ostringstream output;
    bltzr_cmd::ExecutionContext context{transport, session,
                                               bltzr_cmd::ExecutionMode::Batch, output};
    const bltzr_cmd::Result result =
        bltzr_cmd::execute(parseSingle("set_profile balanced"), context);
    ASSERT_TRUE(result.ok);
    EXPECT_EQ(session.config.performanceProfile, "balanced");
    EXPECT_EQ(result.message, "config applied");
    ASSERT_FALSE(transport.commandHistory.empty());
    EXPECT_EQ(transport.commandHistory.front().first, "set_particle_count");
    bool sawReset = false;
    bool sawEnergyMeasure = false;
    for (const auto& entry : transport.commandHistory) {
        if (entry.first == "reset")
            sawReset = true;
        if (entry.first == "set_energy_measure")
            sawEnergyMeasure = true;
    }
    EXPECT_FALSE(sawReset);
    EXPECT_TRUE(sawEnergyMeasure);
}

TEST(ExecutorFlowTest,
     TST_UNT_MODCLI_041_ExportSnapshotWithoutExtensionUsesSessionDefaultFormat)
{
    FakeTransport transport;
    transport.connected = true;
    bltzr_cmd::SessionState session;
    session.config.exportFormat = "vtk_binary";
    std::ostringstream output;
    bltzr_cmd::ExecutionContext context{
        transport, session, bltzr_cmd::ExecutionMode::Interactive, output};
    const bltzr_cmd::Result result =
        bltzr_cmd::execute(parseSingle("export_snapshot outputs/frame"), context);
    ASSERT_TRUE(result.ok);
    ASSERT_EQ(transport.commandHistory.size(), 1u);
    EXPECT_EQ(transport.commandHistory[0].first, "export");
    EXPECT_NE(transport.commandHistory[0].second.find("\"format\":\"vtk_binary\""),
              std::string::npos);
}
} // namespace bltzr_test_module_cli_command_executor_flows_profile_export
