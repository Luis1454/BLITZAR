// File: tests/unit/module_cli/command_catalog.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "command/CommandCatalog.hpp"
#include <gtest/gtest.h>
#include <string>
#include <type_traits>
namespace grav_test_module_cli_command_catalog {
/// Description: Executes the TEST operation.
TEST(CommandCatalogTest, TST_UNT_MODCLI_024_AllSpecsRoundTripByNameAndId)
{
    const std::vector<grav_cmd::CommandSpec>& specs = grav_cmd::CommandCatalog::all();
    /// Description: Executes the ASSERT_FALSE operation.
    ASSERT_FALSE(specs.empty());
    for (const grav_cmd::CommandSpec& spec : specs) {
        const grav_cmd::CommandSpec* byName = grav_cmd::CommandCatalog::findByName(spec.name);
        const grav_cmd::CommandSpec* byId = grav_cmd::CommandCatalog::findById(spec.id);
        /// Description: Executes the ASSERT_NE operation.
        ASSERT_NE(byName, nullptr);
        /// Description: Executes the ASSERT_NE operation.
        ASSERT_NE(byId, nullptr);
        /// Description: Executes the EXPECT_EQ operation.
        EXPECT_EQ(byName->id, spec.id);
        /// Description: Executes the EXPECT_EQ operation.
        EXPECT_EQ(byId->name, spec.name);
    }
}
/// Description: Executes the TEST operation.
TEST(CommandCatalogTest, TST_UNT_MODCLI_025_RenderHelpFormatsRequiredAndOptionalArguments)
{
    const std::string help = grav_cmd::CommandCatalog::renderHelp();
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(help.find("commands:"), std::string::npos);
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(help.find("connect <host> <port>"), std::string::npos);
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(help.find("step [count]"), std::string::npos);
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(help.find("export_snapshot <path> [format]"), std::string::npos);
}
/// Description: Executes the TEST operation.
TEST(CommandCatalogTest, TST_UNT_MODCLI_026_FindByNameRejectsCaseAndWhitespaceVariants)
{
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(grav_cmd::CommandCatalog::findByName("HELP"), nullptr);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(grav_cmd::CommandCatalog::findByName(" help"), nullptr);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(grav_cmd::CommandCatalog::findByName("status "), nullptr);
}
/// Description: Executes the TEST operation.
TEST(CommandCatalogTest, TST_UNT_MODCLI_027_CommandMetadataIsDeterministicAndDocumented)
{
    const std::vector<grav_cmd::CommandSpec>& specs = grav_cmd::CommandCatalog::all();
    for (const grav_cmd::CommandSpec& spec : specs) {
        /// Description: Executes the EXPECT_FALSE operation.
        EXPECT_FALSE(spec.name.empty());
        /// Description: Executes the EXPECT_FALSE operation.
        EXPECT_FALSE(spec.help.empty());
        /// Description: Executes the EXPECT_TRUE operation.
        EXPECT_TRUE(spec.deterministic);
    }
}
/// Description: Executes the TEST operation.
TEST(CommandCatalogTest, TST_UNT_MODCLI_031_FindByIdRejectsUnknownIdentifier)
{
    const std::vector<grav_cmd::CommandSpec>& specs = grav_cmd::CommandCatalog::all();
    typedef std::underlying_type<grav_cmd::CommandId>::type CommandIdBase;
    const grav_cmd::CommandId unknownId =
        static_cast<grav_cmd::CommandId>(static_cast<CommandIdBase>(specs.size()));
    const grav_cmd::CommandSpec* unknown = grav_cmd::CommandCatalog::findById(unknownId);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(unknown, nullptr);
}
/// Description: Executes the TEST operation.
TEST(CommandCatalogTest, TST_UNT_MODCLI_032_RenderHelpStartsWithStableHeaderAndFirstCommand)
{
    const std::string help = grav_cmd::CommandCatalog::renderHelp();
    const std::string::size_type prefixPosition = std::string::size_type{};
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(help.rfind("commands:\n  help", prefixPosition), prefixPosition);
}
} // namespace grav_test_module_cli_command_catalog
