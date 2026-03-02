#include "apps/module-host/module_host_cli.hpp"
#include "apps/module-host/module_host_cli_args.hpp"
#include "apps/module-host/module_host_cli_text.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace grav_test_module_host_args_text {

std::vector<char *> makeArgv(std::vector<std::string> &storage)
{
    std::vector<char *> argv;
    argv.reserve(storage.size());
    for (std::string &value : storage) {
        argv.push_back(value.data());
    }
    return argv;
}

TEST(ModuleHostCliArgsTest, TST_UNT_MODHOST_001_ParseArgsDefaultsAndVariants)
{
    grav_module_host::HostOptions options{};
    std::string error;

    {
        std::vector<std::string> raw = {"host"};
        std::vector<char *> argv = makeArgv(raw);
        ASSERT_TRUE(grav_module_host::ModuleHostCliArgs::parseArgs(
            static_cast<int>(argv.size()),
            argv.data(),
            options,
            error));
        EXPECT_EQ(options.configPath, "simulation.ini");
        EXPECT_EQ(options.moduleSpecifier, "cli");
        EXPECT_FALSE(options.showHelp);
    }

    {
        std::vector<std::string> raw = {"host", "--config=my.ini", "--module", "qt"};
        std::vector<char *> argv = makeArgv(raw);
        ASSERT_TRUE(grav_module_host::ModuleHostCliArgs::parseArgs(
            static_cast<int>(argv.size()),
            argv.data(),
            options,
            error));
        EXPECT_EQ(options.configPath, "my.ini");
        EXPECT_EQ(options.moduleSpecifier, "qt");
    }
}

TEST(ModuleHostCliArgsTest, TST_UNT_MODHOST_002_ParseArgsRejectsUnknownArgument)
{
    grav_module_host::HostOptions options{};
    std::string error;
    std::vector<std::string> raw = {"host", "--bad-arg"};
    std::vector<char *> argv = makeArgv(raw);

    ASSERT_FALSE(grav_module_host::ModuleHostCliArgs::parseArgs(
        static_cast<int>(argv.size()),
        argv.data(),
        options,
        error));
    EXPECT_NE(error.find("unknown argument"), std::string::npos);
}

TEST(ModuleHostCliTextTest, TST_UNT_MODHOST_003_SplitTokensPreservesQuotedSegments)
{
    const std::vector<std::string> tokens =
        grav_module_host::ModuleHostCliText::splitTokens("switch \"qt inproc\" 'cli mode'");
    ASSERT_EQ(tokens.size(), 3u);
    EXPECT_EQ(tokens[0], "switch");
    EXPECT_EQ(tokens[1], "qt inproc");
    EXPECT_EQ(tokens[2], "cli mode");
}

} // namespace grav_test_module_host_args_text
