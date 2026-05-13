/*
 * @file tests/unit/module_cli/command_batch_runner.cpp
 * @author BLITZAR Contributors
 * @project BLITZAR
 * @brief Automated verification assets for BLITZAR quality gates.
 */

#include "command/execution/BatchRunner.hpp"
#include "command/core/Context.hpp"
#include "command/transport/Transport.hpp"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace bltzr_test_module_cli_batch_runner {
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
        (void)outStatus;
        return bltzr_protocol::Response{true, {}, {}};
    }

    bool connectResult = true;
    bool connected = false;
    bltzr_protocol::Response nextCommandResponse{true, {}, {}};
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

TEST(BatchRunnerTest, TST_UNT_MODCLI_020_RunScriptFileReportsOpenError)
{
    FakeTransport transport;
    bltzr_cmd::SessionState session;
    std::ostringstream output;
    bltzr_cmd::ExecutionContext context{transport, session,
                                               bltzr_cmd::ExecutionMode::Batch, output};
    const std::string path =
        (std::filesystem::temp_directory_path() / "bltzr_missing_script_42861.txt").string();
    std::filesystem::remove(path);
    const bltzr_cmd::Result result =
        bltzr_cmd::runScriptFile(path, context);
    ASSERT_FALSE(result.ok);
    EXPECT_NE(result.message.find("failed to open script"), std::string::npos);
}

TEST(BatchRunnerTest, TST_UNT_MODCLI_021_RunScriptFileReportsParserErrors)
{
    FakeTransport transport;
    bltzr_cmd::SessionState session;
    std::ostringstream output;
    bltzr_cmd::ExecutionContext context{transport, session,
                                               bltzr_cmd::ExecutionMode::Batch, output};
    const std::string path = writeTempScript("bltzr_bad_script_42861.txt", "pause\nbogus\n");
    const bltzr_cmd::Result result =
        bltzr_cmd::runScriptFile(path, context);
    std::filesystem::remove(path);
    ASSERT_FALSE(result.ok);
    EXPECT_EQ(result.message, "line 2: unknown command 'bogus'");
}

TEST(BatchRunnerTest, TST_UNT_MODCLI_022_RunScriptFilePrefixesFailingLineNumber)
{
    FakeTransport transport;
    transport.nextCommandResponse = bltzr_protocol::Response{false, {}, "transport down"};
    bltzr_cmd::SessionState session;
    std::ostringstream output;
    bltzr_cmd::ExecutionContext context{transport, session,
                                               bltzr_cmd::ExecutionMode::Batch, output};
    const std::string path = writeTempScript("bltzr_fail_script_42861.txt", "pause\nresume\n");
    const bltzr_cmd::Result result =
        bltzr_cmd::runScriptFile(path, context);
    std::filesystem::remove(path);
    ASSERT_FALSE(result.ok);
    EXPECT_EQ(result.message, "line 1: transport down");
}

TEST(BatchRunnerTest, TST_UNT_MODCLI_023_RunScriptFileSucceedsOnValidCommands)
{
    FakeTransport transport;
    transport.connected = true;
    bltzr_cmd::SessionState session;
    std::ostringstream output;
    bltzr_cmd::ExecutionContext context{transport, session,
                                               bltzr_cmd::ExecutionMode::Batch, output};
    const std::string path =
        writeTempScript("bltzr_ok_script_42861.txt", "pause\nstep 2\nresume\n");
    const bltzr_cmd::Result result =
        bltzr_cmd::runScriptFile(path, context);
    std::filesystem::remove(path);
    ASSERT_TRUE(result.ok);
    EXPECT_TRUE(result.message.empty());
    ASSERT_EQ(transport.commandHistory.size(), 3u);
    EXPECT_EQ(transport.commandHistory[0].first, "pause");
    EXPECT_EQ(transport.commandHistory[1].first, "step");
    EXPECT_EQ(transport.commandHistory[1].second, "\"count\":2");
    EXPECT_EQ(transport.commandHistory[2].first, "resume");
}
} // namespace bltzr_test_module_cli_batch_runner
