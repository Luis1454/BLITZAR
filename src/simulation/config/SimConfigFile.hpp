#ifndef BLITZAR_SIMULATION_CONFIG_SIM_CONFIG_FILE_HPP
#define BLITZAR_SIMULATION_CONFIG_SIM_CONFIG_FILE_HPP

#include <blitzar/blitzar.h>
#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace blitzar_sim {

struct SimConfigFile final {
    struct Value final {
        std::string text;
        bool quoted{};
    };

    struct Argument final {
        std::string name;
        Value value;
    };

    struct Directive final {
        std::string name;
        std::vector<Argument> arguments;
    };

    std::vector<Directive> directives;
};

[[nodiscard]] blitzar_status ParseConfig(
    std::string_view source, SimConfigFile& destination) noexcept;

[[nodiscard]] blitzar_status LoadConfig(
    const std::filesystem::path& path, SimConfigFile& destination) noexcept;

} // namespace blitzar_sim

#endif
