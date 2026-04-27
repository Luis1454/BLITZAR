// File: tests/unit/module_client/cli_args_text.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "apps/client-host/client_host_cli.hpp"
#include "apps/client-host/client_host_cli_args.hpp"
#include "apps/client-host/client_host_cli_text.hpp"
#include "apps/client-host/client_host_module_ops.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include <vector>
namespace grav_test_client_host_args_text {
/// Description: Executes the makeArgv operation.
std::vector<char*> makeArgv(std::vector<std::string>& storage)
{
    std::vector<char*> argv;
    argv.reserve(storage.size());
    for (std::string& value : storage)
        argv.push_back(value.data());
    return argv;
}
/// Description: Executes the TEST operation.
TEST(ClientHostCliArgsTest, TST_UNT_MODHOST_001_ParseArgsDefaultsAndVariants)
{
    grav_client_host::HostOptions options{};
    std::string error;
    {
        std::vector<std::string> raw = {"host"};
        std::vector<char*> argv = makeArgv(raw);
        ASSERT_TRUE(grav_client_host::ClientHostCliArgs::parseArgs(static_cast<int>(argv.size()),
                                                                   argv.data(), options, error));
        /// Description: Executes the EXPECT_EQ operation.
        EXPECT_EQ(options.configPath, "simulation.ini");
        /// Description: Executes the EXPECT_EQ operation.
        EXPECT_EQ(options.moduleSpecifier, "cli");
        /// Description: Executes the EXPECT_FALSE operation.
        EXPECT_FALSE(options.showHelp);
    }
    {
        std::vector<std::string> raw = {"host", "--config=my.ini", "--module",
                                        "qt",   "--validate-only", "--script=batch.dsl"};
        std::vector<char*> argv = makeArgv(raw);
        ASSERT_TRUE(grav_client_host::ClientHostCliArgs::parseArgs(static_cast<int>(argv.size()),
                                                                   argv.data(), options, error));
        /// Description: Executes the EXPECT_EQ operation.
        EXPECT_EQ(options.configPath, "my.ini");
        /// Description: Executes the EXPECT_EQ operation.
        EXPECT_EQ(options.moduleSpecifier, "qt");
        /// Description: Executes the EXPECT_TRUE operation.
        EXPECT_TRUE(options.validateOnly);
        /// Description: Executes the EXPECT_EQ operation.
        EXPECT_EQ(options.scriptPath, "batch.dsl");
        /// Description: Executes the EXPECT_FALSE operation.
        EXPECT_FALSE(options.waitForModule);
    }
    {
        std::vector<std::string> raw = {"host", "--module=qt", "--wait-for-module"};
        std::vector<char*> argv = makeArgv(raw);
        ASSERT_TRUE(grav_client_host::ClientHostCliArgs::parseArgs(static_cast<int>(argv.size()),
                                                                   argv.data(), options, error));
        /// Description: Executes the EXPECT_EQ operation.
        EXPECT_EQ(options.moduleSpecifier, "qt");
        /// Description: Executes the EXPECT_TRUE operation.
        EXPECT_TRUE(options.waitForModule);
    }
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::filesystem::path path = std::filesystem::temp_directory_path() /
                                       ("gravity_validate_only_" + std::to_string(stamp) + ".ini");
    {
        /// Description: Executes the out operation.
        std::ofstream out(path, std::ios::trunc);
        /// Description: Executes the ASSERT_TRUE operation.
        ASSERT_TRUE(out.is_open());
        out << "particle_count=1\n";
        out << "init_config_style=detailed\n";
        out << "init_mode=file\n";
    }
    grav_client_host::HostOptions validateOnly{};
    validateOnly.configPath = path.string();
    validateOnly.validateOnly = true;
    std::ostringstream output;
    std::streambuf* previous = std::cout.rdbuf(output.rdbuf());
    const int exitCode = grav_client_host::ClientHostCli::run(validateOnly, "blitzar-client");
    std::cout.rdbuf(previous);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(exitCode, 3);
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(output.str().find("[preflight] blocked"), std::string::npos);
    std::error_code ec;
    /// Description: Executes the remove operation.
    std::filesystem::remove(path, ec);
}
/// Description: Executes the TEST operation.
TEST(ClientHostCliArgsTest, TST_UNT_MODHOST_002_ParseArgsRejectsUnknownArgument)
{
    grav_client_host::HostOptions options{};
    std::string error;
    std::vector<std::string> raw = {"host", "--bad-arg"};
    std::vector<char*> argv = makeArgv(raw);
    ASSERT_FALSE(grav_client_host::ClientHostCliArgs::parseArgs(static_cast<int>(argv.size()),
                                                                argv.data(), options, error));
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(error.find("unknown argument"), std::string::npos);
}
/// Description: Executes the TEST operation.
TEST(ClientHostCliTextTest, TST_UNT_MODHOST_003_SplitTokensPreservesQuotedSegments)
{
    const std::vector<std::string> tokens =
        /// Description: Executes the splitTokens operation.
        grav_client_host::ClientHostCliText::splitTokens("switch \"qt inproc\" 'cli mode'");
    /// Description: Executes the ASSERT_EQ operation.
    ASSERT_EQ(tokens.size(), 3u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(tokens[0], "switch");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(tokens[1], "qt inproc");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(tokens[2], "cli mode");
}
/// Description: Executes the TEST operation.
TEST(ClientHostCliArgsTest, TST_UNT_MODHOST_007_AliasResolutionReturnsExpectedModuleId)
{
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(grav_client_host::ClientHostModuleOps::expectedModuleIdForSpecifier("qt"), "qt");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(grav_client_host::ClientHostModuleOps::expectedModuleIdForSpecifier("CLI"), "cli");
    EXPECT_TRUE(
        /// Description: Executes the expectedModuleIdForSpecifier operation.
        grav_client_host::ClientHostModuleOps::expectedModuleIdForSpecifier("plugins/custom.dll")
            .empty());
}
/// Description: Executes the TEST operation.
TEST(ClientHostCliArgsTest, TST_UNT_MODHOST_008_HelpReflectsProfileReloadPolicy)
{
    std::ostringstream buffer;
    std::streambuf* previous = std::cout.rdbuf(buffer.rdbuf());
    /// Description: Executes the printHelp operation.
    grav_client_host::ClientHostCliArgs::printHelp("blitzar-client");
    std::cout.rdbuf(previous);
    const std::string rendered = buffer.str();
    if (grav_client_host::ClientHostCli::liveReloadEnabled()) {
        /// Description: Executes the EXPECT_NE operation.
        EXPECT_NE(rendered.find("reload\n"), std::string::npos);
        /// Description: Executes the EXPECT_NE operation.
        EXPECT_NE(rendered.find("switch <module_alias_or_path>\n"), std::string::npos);
    }
    else {
        /// Description: Executes the EXPECT_EQ operation.
        EXPECT_EQ(rendered.find("reload\n"), std::string::npos);
        /// Description: Executes the EXPECT_EQ operation.
        EXPECT_EQ(rendered.find("switch <module_alias_or_path>\n"), std::string::npos);
        /// Description: Executes the EXPECT_NE operation.
        EXPECT_NE(rendered.find("reload is disabled"), std::string::npos);
    }
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(rendered.find("--validate-only"), std::string::npos);
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(rendered.find("--script"), std::string::npos);
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(rendered.find("--wait-for-module"), std::string::npos);
}
/// Description: Executes the TEST operation.
TEST(ClientHostCliArgsTest, TST_UNT_MODHOST_011_BatchScriptRunsDeterministicHelpAndExits)
{
    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::filesystem::path path = std::filesystem::temp_directory_path() /
                                       ("gravity_batch_" + std::to_string(stamp) + ".dsl");
    {
        /// Description: Executes the out operation.
        std::ofstream out(path, std::ios::trunc);
        /// Description: Executes the ASSERT_TRUE operation.
        ASSERT_TRUE(out.is_open());
        out << "help\n";
    }
    grav_client_host::HostOptions options{};
    options.scriptPath = path.string();
    std::ostringstream output;
    std::streambuf* previousOut = std::cout.rdbuf(output.rdbuf());
    const int exitCode = grav_client_host::ClientHostCli::run(options, "blitzar-client");
    std::cout.rdbuf(previousOut);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(exitCode, 0);
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(output.str().find("load_config"), std::string::npos);
    std::error_code ec;
    /// Description: Executes the remove operation.
    std::filesystem::remove(path, ec);
}
} // namespace grav_test_client_host_args_text
