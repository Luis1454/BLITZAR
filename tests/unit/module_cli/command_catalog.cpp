/*
 * @file tests/unit/module_cli/command_catalog.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Automated verification assets for BLITZAR quality gates.
 */

#include "command/CommandCatalog.hpp"
#include <gtest/gtest.h>
#include <string>
#include <type_traits>

namespace bltzr_test_module_cli_command_catalog {
TEST(CommandCatalogTest, TST_UNT_MODCLI_024_AllSpecsRoundTripByNameAndId)
{
    const std::vector<bltzr_cmd::CommandSpec>& specs = bltzr_cmd::CommandCatalog::all();
    ASSERT_FALSE(specs.empty());
    for (const bltzr_cmd::CommandSpec& spec : specs) {
        const bltzr_cmd::CommandSpec* byName = bltzr_cmd::CommandCatalog::findByName(spec.name);
        const bltzr_cmd::CommandSpec* byId = bltzr_cmd::CommandCatalog::findById(spec.id);
        ASSERT_NE(byName, nullptr);
        ASSERT_NE(byId, nullptr);
        EXPECT_EQ(byName->id, spec.id);
        EXPECT_EQ(byId->name, spec.name);
    }
}

TEST(CommandCatalogTest, TST_UNT_MODCLI_025_RenderHelpFormatsRequiredAndOptionalArguments)
{
    const std::string help = bltzr_cmd::CommandCatalog::renderHelp();
    EXPECT_NE(help.find("commands:"), std::string::npos);
    EXPECT_NE(help.find("connect <host> <port>"), std::string::npos);
    EXPECT_NE(help.find("step [count]"), std::string::npos);
    EXPECT_NE(help.find("export_snapshot <path> [format]"), std::string::npos);
}

TEST(CommandCatalogTest, TST_UNT_MODCLI_026_FindByNameRejectsCaseAndWhitespaceVariants)
{
    EXPECT_EQ(bltzr_cmd::CommandCatalog::findByName("HELP"), nullptr);
    EXPECT_EQ(bltzr_cmd::CommandCatalog::findByName(" help"), nullptr);
    EXPECT_EQ(bltzr_cmd::CommandCatalog::findByName("status "), nullptr);
}

TEST(CommandCatalogTest, TST_UNT_MODCLI_027_CommandMetadataIsDeterministicAndDocumented)
{
    const std::vector<bltzr_cmd::CommandSpec>& specs = bltzr_cmd::CommandCatalog::all();
    for (const bltzr_cmd::CommandSpec& spec : specs) {
        EXPECT_FALSE(spec.name.empty());
        EXPECT_FALSE(spec.help.empty());
        EXPECT_TRUE(spec.deterministic);
    }
}

TEST(CommandCatalogTest, TST_UNT_MODCLI_031_FindByIdRejectsUnknownIdentifier)
{
    const std::vector<bltzr_cmd::CommandSpec>& specs = bltzr_cmd::CommandCatalog::all();
    typedef std::underlying_type<bltzr_cmd::CommandId>::type CommandIdBase;
    const bltzr_cmd::CommandId unknownId =
        static_cast<bltzr_cmd::CommandId>(static_cast<CommandIdBase>(specs.size()));
    const bltzr_cmd::CommandSpec* unknown = bltzr_cmd::CommandCatalog::findById(unknownId);
    EXPECT_EQ(unknown, nullptr);
}

TEST(CommandCatalogTest, TST_UNT_MODCLI_032_RenderHelpStartsWithStableHeaderAndFirstCommand)
{
    const std::string help = bltzr_cmd::CommandCatalog::renderHelp();
    const std::string::size_type prefixPosition = std::string::size_type{};
    EXPECT_EQ(help.rfind("commands:\n  help", prefixPosition), prefixPosition);
}
} // namespace bltzr_test_module_cli_command_catalog
