/*
 * @file tests/unit/module_cli/command_catalog.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Automated verification assets for BLITZAR quality gates.
 */

#include "command/catalog/CmdCatalog.hpp"
#include <gtest/gtest.h>
#include <string>
#include <type_traits>

namespace bltzr_test_module_cli_command_catalog {
TEST(CatalogTest, TST_UNT_MODCLI_024_AllSpecsRoundTripByNameAndId)
{
    const std::vector<bltzr_cmd::Spec>& specs = bltzr_cmd::Catalog::all();
    ASSERT_FALSE(specs.empty());
    for (const bltzr_cmd::Spec& spec : specs) {
        const auto byName = bltzr_cmd::Catalog::findByName(spec.name);
        const auto byId = bltzr_cmd::Catalog::findById(spec.id);
        ASSERT_TRUE(byName.has_value());
        ASSERT_TRUE(byId.has_value());
        EXPECT_EQ(byName->get().id, spec.id);
        EXPECT_EQ(byId->get().name, spec.name);
    }
}

TEST(CatalogTest, TST_UNT_MODCLI_025_RenderHelpFormatsRequiredAndOptionalArguments)
{
    const std::string help = bltzr_cmd::Catalog::renderHelp();
    EXPECT_NE(help.find("commands:"), std::string::npos);
    EXPECT_NE(help.find("connect <host> <port>"), std::string::npos);
    EXPECT_NE(help.find("step [count]"), std::string::npos);
    EXPECT_NE(help.find("export_snapshot <path> [format]"), std::string::npos);
}

TEST(CatalogTest, TST_UNT_MODCLI_026_FindByNameRejectsCaseAndWhitespaceVariants)
{
    EXPECT_FALSE(bltzr_cmd::Catalog::findByName("HELP").has_value());
    EXPECT_FALSE(bltzr_cmd::Catalog::findByName(" help").has_value());
    EXPECT_FALSE(bltzr_cmd::Catalog::findByName("status ").has_value());
}

TEST(CatalogTest, TST_UNT_MODCLI_027_CommandMetadataIsDeterministicAndDocumented)
{
    const std::vector<bltzr_cmd::Spec>& specs = bltzr_cmd::Catalog::all();
    for (const bltzr_cmd::Spec& spec : specs) {
        EXPECT_FALSE(spec.name.empty());
        EXPECT_FALSE(spec.help.empty());
        EXPECT_TRUE(spec.deterministic);
    }
}

TEST(CatalogTest, TST_UNT_MODCLI_031_FindByIdRejectsUnknownIdentifier)
{
    const std::vector<bltzr_cmd::Spec>& specs = bltzr_cmd::Catalog::all();
    typedef std::underlying_type<bltzr_cmd::Id>::type IdBase;
    const bltzr_cmd::Id unknownId =
        static_cast<bltzr_cmd::Id>(static_cast<IdBase>(specs.size()));
    const auto unknown = bltzr_cmd::Catalog::findById(unknownId);
    EXPECT_FALSE(unknown.has_value());
}

TEST(CatalogTest, TST_UNT_MODCLI_032_RenderHelpStartsWithStableHeaderAndFirstCommand)
{
    const std::string help = bltzr_cmd::Catalog::renderHelp();
    const std::string::size_type prefixPosition = std::string::size_type{};
    EXPECT_EQ(help.rfind("commands:\n  help", prefixPosition), prefixPosition);
}
} // namespace bltzr_test_module_cli_command_catalog
