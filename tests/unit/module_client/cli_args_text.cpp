#include "apps/client-host/client_host_cli.hpp"
#include "apps/client-host/client_host_cli_args.hpp"
#include "apps/client-host/client_host_cli_text.hpp"
#include "apps/client-host/client_host_module_ops.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace grav_test_client_host_args_text {

std::vector<char *> makeArgv(std::vector<std::string> &storage)
{
    std::vector<char *> argv;
    argv.reserve(storage.size());
    for (std::string &value : storage) {
        argv.push_back(value.data());
    }
    return argv;
}

TEST(ClientHostCliArgsTest, TST_UNT_MODHOST_001_ParseArgsDefaultsAndVariants)
{
    grav_client_host::HostOptions options{};
    std::string error;

    {
        std::vector<std::string> raw = {"host"};
        std::vector<char *> argv = makeArgv(raw);
        ASSERT_TRUE(grav_client_host::ClientHostCliArgs::parseArgs(
            static_cast<int>(argv.size()),
            argv.data(),
            options,
            error));
        EXPECT_EQ(options.configPath, "simulation.ini");
        EXPECT_EQ(options.moduleSpecifier, "cli");
        EXPECT_FALSE(options.showHelp);
    }

    {
        std::vector<std::string> raw = {"host", "--config=my.ini", "--module", "qt", "--validate-only"};
        std::vector<char *> argv = makeArgv(raw);
        ASSERT_TRUE(grav_client_host::ClientHostCliArgs::parseArgs(
            static_cast<int>(argv.size()),
            argv.data(),
            options,
            error));
        EXPECT_EQ(options.configPath, "my.ini");
        EXPECT_EQ(options.moduleSpecifier, "qt");
        EXPECT_TRUE(options.validateOnly);
    }

    const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / ("gravity_validate_only_" + std::to_string(stamp) + ".ini");
    {
        std::ofstream out(path, std::ios::trunc);
        ASSERT_TRUE(out.is_open());
        out << "particle_count=1\n";
        out << "init_config_style=detailed\n";
        out << "init_mode=file\n";
    }
    grav_client_host::HostOptions validateOnly{};
    validateOnly.configPath = path.string();
    validateOnly.validateOnly = true;
    std::ostringstream output;
    std::streambuf *previous = std::cout.rdbuf(output.rdbuf());
    const int exitCode = grav_client_host::ClientHostCli::run(validateOnly, "blitzar-client");
    std::cout.rdbuf(previous);
    EXPECT_EQ(exitCode, 3);
    EXPECT_NE(output.str().find("[preflight] blocked"), std::string::npos);
    std::error_code ec;
    std::filesystem::remove(path, ec);
}

TEST(ClientHostCliArgsTest, TST_UNT_MODHOST_002_ParseArgsRejectsUnknownArgument)
{
    grav_client_host::HostOptions options{};
    std::string error;
    std::vector<std::string> raw = {"host", "--bad-arg"};
    std::vector<char *> argv = makeArgv(raw);

    ASSERT_FALSE(grav_client_host::ClientHostCliArgs::parseArgs(
        static_cast<int>(argv.size()),
        argv.data(),
        options,
        error));
    EXPECT_NE(error.find("unknown argument"), std::string::npos);
}

TEST(ClientHostCliTextTest, TST_UNT_MODHOST_003_SplitTokensPreservesQuotedSegments)
{
    const std::vector<std::string> tokens =
        grav_client_host::ClientHostCliText::splitTokens("switch \"qt inproc\" 'cli mode'");
    ASSERT_EQ(tokens.size(), 3u);
    EXPECT_EQ(tokens[0], "switch");
    EXPECT_EQ(tokens[1], "qt inproc");
    EXPECT_EQ(tokens[2], "cli mode");
}

TEST(ClientHostCliArgsTest, TST_UNT_MODHOST_007_AliasResolutionReturnsExpectedModuleId)
{
    EXPECT_EQ(grav_client_host::ClientHostModuleOps::expectedModuleIdForSpecifier("qt"), "qt");
    EXPECT_EQ(grav_client_host::ClientHostModuleOps::expectedModuleIdForSpecifier("CLI"), "cli");
    EXPECT_TRUE(grav_client_host::ClientHostModuleOps::expectedModuleIdForSpecifier("plugins/custom.dll").empty());
}

TEST(ClientHostCliArgsTest, TST_UNT_MODHOST_008_HelpReflectsProfileReloadPolicy)
{
    std::ostringstream buffer;
    std::streambuf *previous = std::cout.rdbuf(buffer.rdbuf());
    grav_client_host::ClientHostCliArgs::printHelp("blitzar-client");
    std::cout.rdbuf(previous);
    const std::string rendered = buffer.str();
    if (grav_client_host::ClientHostCli::liveReloadEnabled()) {
        EXPECT_NE(rendered.find("reload\n"), std::string::npos);
        EXPECT_NE(rendered.find("switch <module_alias_or_path>\n"), std::string::npos);
    } else {
        EXPECT_EQ(rendered.find("reload\n"), std::string::npos);
        EXPECT_EQ(rendered.find("switch <module_alias_or_path>\n"), std::string::npos);
        EXPECT_NE(rendered.find("reload is disabled"), std::string::npos);
    }
    EXPECT_NE(rendered.find("--validate-only"), std::string::npos);
}

} // namespace grav_test_client_host_args_text
