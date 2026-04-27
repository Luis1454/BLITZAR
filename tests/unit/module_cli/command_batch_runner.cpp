// File: tests/unit/module_cli/command_batch_runner.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "command/CommandBatchRunner.hpp"
#include "command/CommandContext.hpp"
#include "command/CommandTransport.hpp"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
namespace grav_test_module_cli_batch_runner {
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
        (void)outStatus;
        return ServerClientResponse{true, {}, {}};
    }
    bool connectResult = true;
    bool connected = false;
    ServerClientResponse nextCommandResponse{true, {}, {}};
    std::vector<std::pair<std::string, std::uint16_t>> connectHistory;
    std::vector<std::pair<std::string, std::string>> commandHistory;
};
/// Description: Executes the writeTempScript operation.
static std::string writeTempScript(const std::string& name, const std::string& content)
{
    const std::filesystem::path path = std::filesystem::temp_directory_path() / name;
    /// Description: Executes the out operation.
    std::ofstream out(path.string(), std::ios::binary);
    out << content;
    out.close();
    return path.string();
}
/// Description: Executes the TEST operation.
TEST(CommandBatchRunnerTest, TST_UNT_MODCLI_020_RunScriptFileReportsOpenError)
{
    FakeCommandTransport transport;
    grav_cmd::CommandSessionState session;
    std::ostringstream output;
    grav_cmd::CommandExecutionContext context{transport, session,
                                              grav_cmd::CommandExecutionMode::Batch, output};
    const std::string path =
        (std::filesystem::temp_directory_path() / "grav_missing_script_42861.txt").string();
    /// Description: Executes the remove operation.
    std::filesystem::remove(path);
    const grav_cmd::CommandResult result =
        /// Description: Executes the runScriptFile operation.
        grav_cmd::CommandBatchRunner::runScriptFile(path, context);
    /// Description: Executes the ASSERT_FALSE operation.
    ASSERT_FALSE(result.ok);
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(result.message.find("failed to open script"), std::string::npos);
}
/// Description: Executes the TEST operation.
TEST(CommandBatchRunnerTest, TST_UNT_MODCLI_021_RunScriptFileReportsParserErrors)
{
    FakeCommandTransport transport;
    grav_cmd::CommandSessionState session;
    std::ostringstream output;
    grav_cmd::CommandExecutionContext context{transport, session,
                                              grav_cmd::CommandExecutionMode::Batch, output};
    const std::string path = writeTempScript("grav_bad_script_42861.txt", "pause\nbogus\n");
    const grav_cmd::CommandResult result =
        /// Description: Executes the runScriptFile operation.
        grav_cmd::CommandBatchRunner::runScriptFile(path, context);
    /// Description: Executes the remove operation.
    std::filesystem::remove(path);
    /// Description: Executes the ASSERT_FALSE operation.
    ASSERT_FALSE(result.ok);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(result.message, "line 2: unknown command 'bogus'");
}
/// Description: Executes the TEST operation.
TEST(CommandBatchRunnerTest, TST_UNT_MODCLI_022_RunScriptFilePrefixesFailingLineNumber)
{
    FakeCommandTransport transport;
    transport.nextCommandResponse = ServerClientResponse{false, {}, "transport down"};
    grav_cmd::CommandSessionState session;
    std::ostringstream output;
    grav_cmd::CommandExecutionContext context{transport, session,
                                              grav_cmd::CommandExecutionMode::Batch, output};
    const std::string path = writeTempScript("grav_fail_script_42861.txt", "pause\nresume\n");
    const grav_cmd::CommandResult result =
        /// Description: Executes the runScriptFile operation.
        grav_cmd::CommandBatchRunner::runScriptFile(path, context);
    /// Description: Executes the remove operation.
    std::filesystem::remove(path);
    /// Description: Executes the ASSERT_FALSE operation.
    ASSERT_FALSE(result.ok);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(result.message, "line 1: transport down");
}
/// Description: Executes the TEST operation.
TEST(CommandBatchRunnerTest, TST_UNT_MODCLI_023_RunScriptFileSucceedsOnValidCommands)
{
    FakeCommandTransport transport;
    transport.connected = true;
    grav_cmd::CommandSessionState session;
    std::ostringstream output;
    grav_cmd::CommandExecutionContext context{transport, session,
                                              grav_cmd::CommandExecutionMode::Batch, output};
    const std::string path = writeTempScript("grav_ok_script_42861.txt", "pause\nstep 2\nresume\n");
    const grav_cmd::CommandResult result =
        /// Description: Executes the runScriptFile operation.
        grav_cmd::CommandBatchRunner::runScriptFile(path, context);
    /// Description: Executes the remove operation.
    std::filesystem::remove(path);
    /// Description: Executes the ASSERT_TRUE operation.
    ASSERT_TRUE(result.ok);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(result.message.empty());
    /// Description: Executes the ASSERT_EQ operation.
    ASSERT_EQ(transport.commandHistory.size(), 3u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(transport.commandHistory[0].first, "pause");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(transport.commandHistory[1].first, "step");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(transport.commandHistory[1].second, "\"count\":2");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(transport.commandHistory[2].first, "resume");
}
} // namespace grav_test_module_cli_batch_runner
