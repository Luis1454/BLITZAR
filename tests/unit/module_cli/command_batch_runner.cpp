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
        (void)outStatus;
        return ServerClientResponse{true, {}, {}};
    }
    bool connectResult = true;
    bool connected = false;
    ServerClientResponse nextCommandResponse{true, {}, {}};
    std::vector<std::pair<std::string, std::uint16_t>> connectHistory;
    std::vector<std::pair<std::string, std::string>> commandHistory;
};
static std::string writeTempScript(const std::string& name, const std::string& content)
{
    const std::filesystem::path path = std::filesystem::temp_directory_path() / name;
    std::ofstream out(path.string(), std::ios::binary);
    out << content;
    out.close();
    return path.string();
}
TEST(CommandBatchRunnerTest, TST_UNT_MODCLI_020_RunScriptFileReportsOpenError)
{
    FakeCommandTransport transport;
    grav_cmd::CommandSessionState session;
    std::ostringstream output;
    grav_cmd::CommandExecutionContext context{transport, session,
                                              grav_cmd::CommandExecutionMode::Batch, output};
    const std::string path =
        (std::filesystem::temp_directory_path() / "grav_missing_script_42861.txt").string();
    std::filesystem::remove(path);
    const grav_cmd::CommandResult result =
        grav_cmd::CommandBatchRunner::runScriptFile(path, context);
    ASSERT_FALSE(result.ok);
    EXPECT_NE(result.message.find("failed to open script"), std::string::npos);
}
TEST(CommandBatchRunnerTest, TST_UNT_MODCLI_021_RunScriptFileReportsParserErrors)
{
    FakeCommandTransport transport;
    grav_cmd::CommandSessionState session;
    std::ostringstream output;
    grav_cmd::CommandExecutionContext context{transport, session,
                                              grav_cmd::CommandExecutionMode::Batch, output};
    const std::string path = writeTempScript("grav_bad_script_42861.txt", "pause\nbogus\n");
    const grav_cmd::CommandResult result =
        grav_cmd::CommandBatchRunner::runScriptFile(path, context);
    std::filesystem::remove(path);
    ASSERT_FALSE(result.ok);
    EXPECT_EQ(result.message, "line 2: unknown command 'bogus'");
}
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
        grav_cmd::CommandBatchRunner::runScriptFile(path, context);
    std::filesystem::remove(path);
    ASSERT_FALSE(result.ok);
    EXPECT_EQ(result.message, "line 1: transport down");
}
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
        grav_cmd::CommandBatchRunner::runScriptFile(path, context);
    std::filesystem::remove(path);
    ASSERT_TRUE(result.ok);
    EXPECT_TRUE(result.message.empty());
    ASSERT_EQ(transport.commandHistory.size(), 3u);
    EXPECT_EQ(transport.commandHistory[0].first, "pause");
    EXPECT_EQ(transport.commandHistory[1].first, "step");
    EXPECT_EQ(transport.commandHistory[1].second, "\"count\":2");
    EXPECT_EQ(transport.commandHistory[2].first, "resume");
}
} // namespace grav_test_module_cli_batch_runner
