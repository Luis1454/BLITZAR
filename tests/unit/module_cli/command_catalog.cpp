/*
 * @file tests/unit/module_cli/command_catalog.cpp
 * @author BLITZAR Contributors
 * @project BLITZAR
 * @brief Automated verification assets for BLITZAR quality gates.
 */

#include "command/catalog/Catalog.hpp"
#include <gtest/gtest.h>
#include <string>
#include <type_traits>

namespace bltzr_test_module_cli_command_catalog {
TEST(CatalogTest, TST_UNT_MODCLI_024_AllSpecsRoundTripByNameAndId)
{
    const std::vector<bltzr_cmd::Spec>& specs = bltzr_cmd::Catalog::all();
    ASSERT_FALSE(specs.empty());
    for (const bltzr_cmd::Spec& spec : specs) {
        const bltzr_cmd::Spec* byName = bltzr_cmd::Catalog::findByName(spec.name);
        const bltzr_cmd::Spec* byId = bltzr_cmd::Catalog::findById(spec.id);
        ASSERT_NE(byName, nullptr);
        ASSERT_NE(byId, nullptr);
        EXPECT_EQ(byName->id, spec.id);
        EXPECT_EQ(byId->name, spec.name);
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
    EXPECT_EQ(bltzr_cmd::Catalog::findByName("HELP"), nullptr);
    EXPECT_EQ(bltzr_cmd::Catalog::findByName(" help"), nullptr);
    EXPECT_EQ(bltzr_cmd::Catalog::findByName("status "), nullptr);
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
    const bltzr_cmd::Spec* unknown = bltzr_cmd::Catalog::findById(unknownId);
    EXPECT_EQ(unknown, nullptr);
}

TEST(CatalogTest, TST_UNT_MODCLI_032_RenderHelpStartsWithStableHeaderAndFirstCommand)
{
    const std::string help = bltzr_cmd::Catalog::renderHelp();
    const std::string::size_type prefixPosition = std::string::size_type{};
    EXPECT_EQ(help.rfind("commands:\n  help", prefixPosition), prefixPosition);
}
} // namespace bltzr_test_module_cli_command_catalog
