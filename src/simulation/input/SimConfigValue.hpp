#ifndef BLITZAR_SIMULATION_INPUT_SIM_CONFIG_VALUE_HPP
#define BLITZAR_SIMULATION_INPUT_SIM_CONFIG_VALUE_HPP

#include "simulation/input/SimConfigFile.hpp"

#include <cstdint>
#include <span>
#include <string_view>

namespace blitzar_sim {

[[nodiscard]] bool HasExactArguments(
    const SimConfigFile::Directive& directive, std::span<const std::string_view> expected) noexcept;

[[nodiscard]] bool ReadConfigText(const SimConfigFile::Directive& directive, std::string_view name,
    std::string_view& value) noexcept;

[[nodiscard]] bool ReadConfigInteger(
    const SimConfigFile::Directive& directive, std::string_view name, std::int64_t& value) noexcept;

[[nodiscard]] bool ReadConfigUnsigned(const SimConfigFile::Directive& directive,
    std::string_view name, std::uint64_t& value) noexcept;

[[nodiscard]] bool ReadConfigReal(
    const SimConfigFile::Directive& directive, std::string_view name, double& value) noexcept;

[[nodiscard]] bool ReadConfigBoolean(
    const SimConfigFile::Directive& directive, std::string_view name, bool& value) noexcept;

} // namespace blitzar_sim

#endif
