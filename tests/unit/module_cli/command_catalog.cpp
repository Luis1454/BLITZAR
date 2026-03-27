#include "command/CommandCatalog.hpp"

#include <gtest/gtest.h>

#include <string>
#include <type_traits>

namespace grav_test_module_cli_command_catalog {

TEST(CommandCatalogTest, TST_UNT_MODCLI_024_AllSpecsRoundTripByNameAndId)
{
    const std::vector<grav_cmd::CommandSpec> &specs = grav_cmd::CommandCatalog::all();
    ASSERT_FALSE(specs.empty());

    for (const grav_cmd::CommandSpec &spec : specs) {
        const grav_cmd::CommandSpec *byName = grav_cmd::CommandCatalog::findByName(spec.name);
        const grav_cmd::CommandSpec *byId = grav_cmd::CommandCatalog::findById(spec.id);
        ASSERT_NE(byName, nullptr);
        ASSERT_NE(byId, nullptr);
        EXPECT_EQ(byName->id, spec.id);
        EXPECT_EQ(byId->name, spec.name);
    }
}

TEST(CommandCatalogTest, TST_UNT_MODCLI_025_RenderHelpFormatsRequiredAndOptionalArguments)
{
    const std::string help = grav_cmd::CommandCatalog::renderHelp();

    EXPECT_NE(help.find("commands:"), std::string::npos);
    EXPECT_NE(help.find("connect <host> <port>"), std::string::npos);
    EXPECT_NE(help.find("step [count]"), std::string::npos);
    EXPECT_NE(help.find("export_snapshot <path> [format]"), std::string::npos);
}

TEST(CommandCatalogTest, TST_UNT_MODCLI_026_FindByNameRejectsCaseAndWhitespaceVariants)
{
    EXPECT_EQ(grav_cmd::CommandCatalog::findByName("HELP"), nullptr);
    EXPECT_EQ(grav_cmd::CommandCatalog::findByName(" help"), nullptr);
    EXPECT_EQ(grav_cmd::CommandCatalog::findByName("status "), nullptr);
}

TEST(CommandCatalogTest, TST_UNT_MODCLI_027_CommandMetadataIsDeterministicAndDocumented)
{
    const std::vector<grav_cmd::CommandSpec> &specs = grav_cmd::CommandCatalog::all();
    for (const grav_cmd::CommandSpec &spec : specs) {
        EXPECT_FALSE(spec.name.empty());
        EXPECT_FALSE(spec.help.empty());
        EXPECT_TRUE(spec.deterministic);
    }
}

TEST(CommandCatalogTest, TST_UNT_MODCLI_031_FindByIdRejectsUnknownIdentifier)
{
    const std::vector<grav_cmd::CommandSpec> &specs = grav_cmd::CommandCatalog::all();
    using CommandIdBase = std::underlying_type_t<grav_cmd::CommandId>;
    const grav_cmd::CommandId unknownId = static_cast<grav_cmd::CommandId>(static_cast<CommandIdBase>(specs.size()));
    const grav_cmd::CommandSpec *unknown = grav_cmd::CommandCatalog::findById(unknownId);
    EXPECT_EQ(unknown, nullptr);
}

TEST(CommandCatalogTest, TST_UNT_MODCLI_032_RenderHelpStartsWithStableHeaderAndFirstCommand)
{
    const std::string help = grav_cmd::CommandCatalog::renderHelp();
    const std::string::size_type prefixPosition = std::string::size_type{};
    EXPECT_EQ(help.rfind("commands:\n  help", prefixPosition), prefixPosition);
}

} // namespace grav_test_module_cli_command_catalog
