#include "simulation/input/SimConfigFile.hpp"

#include "fixtures/FixtureCheck.hpp"

#include <array>
#include <cstdio>
#include <filesystem>
#include <string_view>

namespace {

int CheckInlineSyntax()
{
    constexpr std::string_view source = R"(# A directive file has one directive per line.
simulation(particle_count=4, dt=0.01, solver=direct, title="clean room")
object(id=first, name="First") # Inline comments are allowed.
object(id=second, name="")
)";

    blitzar_sim::SimConfigFile config;

    BLITZAR_CHECK(blitzar_sim::ParseConfig(source, config) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(config.directives.size() == 3U);
    BLITZAR_CHECK(config.directives[0].name == "simulation");
    BLITZAR_CHECK(config.directives[0].arguments.size() == 4U);
    BLITZAR_CHECK(config.directives[0].arguments[0].name == "particle_count");
    BLITZAR_CHECK(config.directives[0].arguments[0].value.text == "4");
    BLITZAR_CHECK(!config.directives[0].arguments[0].value.quoted);
    BLITZAR_CHECK(config.directives[0].arguments[3].value.text == "clean room");
    BLITZAR_CHECK(config.directives[0].arguments[3].value.quoted);
    BLITZAR_CHECK(config.directives[1].name == "object");
    BLITZAR_CHECK(config.directives[2].name == "object");
    BLITZAR_CHECK(config.directives[2].arguments[1].value.text.empty());
    BLITZAR_CHECK(config.directives[2].arguments[1].value.quoted);

    return 0;
}

int CheckMalformedSyntax()
{
    constexpr std::array<std::string_view, 6> invalid_sources{
        "simulation(particle_count)",
        "simulation(particle_count=)",
        "simulation(name=\"unterminated)",
        "simulation(particle_count=4,)",
        "simulation(particle_count=4) trailing",
        "simulation(particle_count=4",
    };

    blitzar_sim::SimConfigFile config;

    BLITZAR_CHECK(blitzar_sim::ParseConfig("valid(value=1)", config) == BLITZAR_STATUS_OK);

    for (const std::string_view source : invalid_sources) {
        BLITZAR_CHECK(blitzar_sim::ParseConfig(source, config) == BLITZAR_STATUS_INVALID_ARGUMENT);
    }

    BLITZAR_CHECK(config.directives.size() == 1U);
    BLITZAR_CHECK(config.directives[0].name == "valid");

    return 0;
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 2) {
        (void)std::fprintf(stderr, "usage: blitzar_config_test <config-file>\n");

        return 2;
    }

    BLITZAR_CHECK(CheckInlineSyntax() == 0);
    BLITZAR_CHECK(CheckMalformedSyntax() == 0);

    blitzar_sim::SimConfigFile config;

    BLITZAR_CHECK(
        blitzar_sim::LoadConfig(std::filesystem::path(argv[1]), config) == BLITZAR_STATUS_OK);

    BLITZAR_CHECK(!config.directives.empty());

    for (const blitzar_sim::SimConfigFile::Directive& directive : config.directives) {
        BLITZAR_CHECK(!directive.name.empty());

        for (const blitzar_sim::SimConfigFile::Argument& argument : directive.arguments) {
            BLITZAR_CHECK(!argument.name.empty());
        }
    }

    (void)std::fprintf(
        stdout, "BLITZAR CONFIG syntax=directive-v1 directives=%zu\n", config.directives.size());

    return 0;
}
